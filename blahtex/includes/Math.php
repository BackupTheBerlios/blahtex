<?php
/**
 * @file Math.php
 * Contains everything related to <math> </math> parsing.
 * \sa math/README
 */

/**
 * %Parser for the blahtex's output. 
 */
class blahtexOutputParser  {
   var $parser;  /**< \private */
	var $stack;   /**< \private */
	var $results; /**< \private */

	function blahtexOutputParser()
	{
		$this->parser = xml_parser_create( "UTF-8" );
		$this->stack = array();
		$this->results = array();
		$this->prevCdata = false;
		
		xml_set_object( $this->parser, $this );
		xml_parser_set_option( $this->parser, XML_OPTION_CASE_FOLDING, 0 );
		xml_set_element_handler( $this->parser, "startElement", "stopElement" );
		xml_set_character_data_handler( $this->parser, "characterData" );
	}

	/**
	 * Main function, which parses blahtex's output.
	 * The format of blahtex's output is based on XML. This function
	 * parses the XML and returns an array representing the tree
	 * structure. For instance, if $retval denotes the return value,
	 * then $retval["blahtex"]["error"] contains the text within the
	 * <error> tag within the <blahtex> tag. If there is more than one
	 * <error> tag within a <blahtex> tag, then
	 * $retval["blahtex"]["error"] is an array of strings. As a special
	 * case, $retval["mathmlMarkup"] contains the segment between
	 * <markup> and </markup>.
	 * @param $data String with output to be parsed.
	 * @return Array representing XML tree.
	 */
	function parse( $data )
	{
		// We splice out any segment between <markup> and </markup>  
		// so that the XML parser doesn't have to deal with all the MathML tags.
		$markupBegin = strpos( $data, "<markup>" );
		if ( !( $markupBegin === false ) ) {
			$markupEnd = strpos( $data, "</markup>" );
			$this->results["mathmlMarkup"] = 
				trim( substr( $data, $markupBegin + 8, $markupEnd - $markupBegin - 8 ) );
			$data = substr( $data, 0, $markupBegin + 8 ) . substr( $data, $markupEnd );
		}
		xml_parse( $this->parser, $data );
		return $this->results;
	}

	/** @privatesection */
	function startElement( $parser, $name, $attributes )
	{
		$this->prevCdata = false;
		if ( count( $this->stack ) == 0 )
			array_push( $this->stack, $name );
		else
			array_push( $this->stack, $this->stack[count( $this->stack ) - 1] . ":$name" );
	}
	
	function stopElement($parser, $name)
	{
		$this->prevCdata = false;
		array_pop( $this->stack );
	}
	
	function characterData($parser, $data)
	{
		$index = $this->stack[count( $this->stack ) - 1];
		if ( $this->prevCdata ) {
			// Merge subsequent CDATA blocks
			if ( is_array( $this->results[$index] ) )
				array_push( $this->results[$index], 
					    array_pop( $this->results[$index] ) . $data);
			else
				$this->results[$index] .= $data;
		} else {
			if ( !isset( $this->results[$index] ) ) 
				$this->results[$index] = $data;
			elseif ( is_array( $this->results[$index] ) )
				array_push( $this->results[$index], $data );
			else
				$this->results[$index] = array( $this->results[$index], $data );
		}
		$this->prevCdata = true;
	}
}

/**
 * Render formulas to PNG, HTML and MathML.
 * Takes LaTeX fragments, sends them to helper program (texvc and
 * blahtex) for rendering to rasterized PNG and HTML and MathML
 * approximations. An appropriate rendering form is picked, depending
 * on the user's preferences, and returned. The rendering is cached in
 * the @c math table in the database, and the PNG files are cached in
 * @c $wgMathDirectory on the file system.
 *
 * @author Tomasz Wegrzanowski, with additions by Brion Vibber (2003, 2004)
 */
class MathRenderer {
	/** @privatesection */
   var $mode = MW_MATH_MODERN; /**< @User preference for maths */
	var $tex = '';              /**< LaTeX fragment */
	var $inputhash = '';        /**< Hash value of $tex */
	var $hash = '';             /**< Name of PNG file */
	var $html = '';             /**< HTML rendering of $tex */
	var $mathml = '';           /**< MathML rendering of $tex */
	var $conservativeness = 0;  /**< How conservative the HTML rendering is */
	
	/**
	 * Constructor.
	 * @param $tex String containing LaTeX fragment to be rendered.
	 * @public 
	 */
	function MathRenderer( $tex ) {
		$this->tex = $tex;
	 }
	
	/**
	 * Set the preferred output mode.
	 * The output mode specifies whether render() should output PNG,
	 * HTML or MathML.
	 * @param $mode Output mode, can be @c MW_MATH_PNG, 
	 *    @c MW_MATH_SIMPLE, @c MW_MATH_HTML, @c MW_MATH_SOURCE,
	 *    @c MW_MATH_MODERN, or @c MW_MATH_MATHML.
	 * @public
	 */
	function setOutputMode( $mode ) {
		$this->mode = $mode;
	}

	/**
	 * Main function, which renders the LaTeX fragment.
	 * This function renders the LaTeX fragment specified in the
	 * constructor. The output depends on the output mode, set with
	 * setOutputMode(), as follows:
	 *  - @c MW_MATH_PNG : Output is in PNG format, fall back to HTML.
	 *  - @c MW_MATH_SIMPLE : Output is in HTML format if the HTML is
	 *       simple and in PNG otherwise.
	 *  - @c MW_MATH_HTML : Output is in HTML format, fall back to PNG.
	 *  - @c MW_MATH_SOURCE : Output the LaTeX fragment verbatim,
	 *       surrounded by a pair of @c $ characters.
	 *  - @c MW_MATH_MODERN : Output is in HTML format unless the HTML
	 *       is complicated, fall back to PNG.
	 *  - @c MW_MATH_MATHML : Output is in MathML format, fall back to
	 *       PNG.
	 *
    * @return String containing HTML fragment, representing the
    * formula in the given LaTeX fragment.
	 * @public
	 */
	function render() {
		global $wgBlahtex;
		$fname = 'MathRenderer::render';
	
		if( $this->mode == MW_MATH_SOURCE ) {
			# No need to render or parse anything more!
			return ( '$ '.htmlspecialchars( $this->tex ).' $' );
		}
		
		if( !$this->_recall() ) {
			$res = $this->testEnvironment();
			if ( $res )
				return $res;
			
			// Run texvc
			list( $success, $res ) = $this->invokeTexvc( $this->tex );
			if ( !$success )
				return $res;
			$texvcError = $this->processTexvcOutput( $res );
			
			// Run blahtex, if configured
			if ( $wgBlahtex ) {
				list( $success, $res ) = $this->invokeBlahtex( $this->tex, $this->hash == NULL );
				if ( !$success )
					return $res;
				$parser = new blahtexOutputParser();
				$res = $parser->parse( $res );
				wfDebug(print_r($res, TRUE));
				$blahtexError = $this->processBlahtexOutput( $res );
				if ( $blahtexError && $texvcError )
					return $blahtexError;
			} else {
				if ( $texvcError )
					return $texvcError;
			}

			# Now save it back to the DB:
			if ( !wfReadOnly() ) {
				if ( $this->hash )
					$outmd5_sql = pack( 'H32', $this->hash );
				else
					$outmd5_sql = '';
			
				$md5_sql = pack( 'H32', $this->md5 ); # Binary packed, not hex
				
				$dbw =& wfGetDB( DB_MASTER );
				$dbw->replace( 'math', array( 'math_inputhash' ),
				  array( 
					'math_inputhash' => $md5_sql, 
					'math_outputhash' => $outmd5_sql,
					'math_html_conservativeness' => $this->conservativeness,
					'math_html' => $this->html,
					'math_mathml' => $this->mathml,
				  ), $fname, array( 'IGNORE' ) 
				);
			}
		}
		  
		return $this->_doRender();
	}

	/**
	 * Test whether the necessary directories and executables exist.
	 * @return String containing HTML fragment with error message if
	 * there is a problem, @c false otherwise.
	 */
	function testEnvironment()
	{
		global $wgTmpDirectory, $wgTexvc, $wgBlahtex;
		
		if( !file_exists( $wgTmpDirectory ) ) {
			if( !@mkdir( $wgTmpDirectory ) ) {
				return $this->_error( 'math_bad_tmpdir' );
			}
		} elseif( !is_dir( $wgTmpDirectory ) || !is_writable( $wgTmpDirectory ) ) {
			return $this->_error( 'math_bad_tmpdir' );
		}
		
		if( function_exists( 'is_executable' ) && !is_executable( $wgTexvc ) ) {
			return $this->_error( 'math_notexvc' );
		}
		if ($wgBlahtex && function_exists( 'is_executable' ) && !is_executable( $wgBlahtex ))
			return $this->_error( 'math_noblahtex', $wgBlahtex );
		
		return false;
	}

	/**
	 * Invoke the texvc executable.
	 * This function invokes the @c texvc helper program, whose
	 * location is specified in $wgTexvc. 
	 * @param $tex String containing the LaTeX fragment to be rendered.
	 * @return A 2-tuple. 
	 *  - If an error occurred, then the first element is @c false and
	 *    the second element is a string containing an HTML fragment
	 *    with the error message.
	 *  - Otherwise, the first element is @c true and the second
	 *    element s a string containing the output of @c texvc.
	 */
	function invokeTexvc( $tex )
	{
		global $wgMathDirectory, $wgTmpDirectory, $wgTexvc, $wgInputEncoding;

		$cmd = $wgTexvc . ' ' . 
			escapeshellarg( $wgTmpDirectory ).' '.
			escapeshellarg( $wgTmpDirectory ).' '.
			escapeshellarg( $this->tex ).' '.
			escapeshellarg( $wgInputEncoding );
					
		if ( wfIsWindows() ) {
			// Invoke it within cygwin sh, because texvc expects sh features in its default shell
			$cmd = 'sh -c ' . wfEscapeShellArg( $cmd );
		} 
		
		wfDebug( "TeX: $cmd\n" );
		$contents = `$cmd`;
		wfDebug( "TeX output:\n $contents\n---\n" );
		
		if ( strlen( $contents ) == 0 ) {
			return array( false, $this->_error( 'math_unknown_error' ) );
		}

		return array( true, $contents );
	}

	/**
	 * Process texvc output.
	 * Parse the output, fill the mathml, html, hash, and
	 * conservativeness fields in the database and move the PNG image
	 * to its final destination. 
	 * @param $contents String containing texvc output.
	 * @return String containing HTML fragment with error message if
	 * an error occurred, @c false otherwise.
	 */
	function processTexvcOutput( $contents ) {
		global $wgTmpDirectory;

		$retval = substr( $contents, 0, 1 );
		if ( ( $retval == 'C' ) || ( $retval == 'M' ) || ( $retval == 'L' ) ) {
			if ( $retval == 'C' )
				$this->conservativeness = 2;
			else if ( $retval == 'M' )
				$this->conservativeness = 1;
			else
				$this->conservativeness = 0;
			$outdata = substr( $contents, 33 );
			
			$i = strpos( $outdata, "\000" );
			
			$this->html = substr( $outdata, 0, $i );
			$this->mathml = substr( $outdata, $i+1 );
		} else if ( ( $retval == 'c' ) || ( $retval == 'm' ) || ( $retval == 'l' ) )  {
			$this->html = substr( $contents, 33 );
			if ( $retval == 'c' )
				$this->conservativeness = 2;
			else if ( $retval == 'm' )
				$this->conservativeness = 1;
			else
				$this->conservativeness = 0;
			$this->mathml = NULL;
		} else if ( $retval == 'X' ) {
			$this->html = NULL;
			$this->mathml = substr( $contents, 33 );
			$this->conservativeness = 0;
		} else if ( $retval == '+' ) {
			$this->html = NULL;
			$this->mathml = NULL;
			$this->conservativeness = 0;
		} else {
			$errbit = htmlspecialchars( substr( $contents, 1 ) );
			switch( $retval ) {
			case 'E': return $this->_error( 'math_lexing_error', $errbit );
			case 'S': return $this->_error( 'math_syntax_error', $errbit );
			case 'F': return $this->_error( 'math_unknown_function', $errbit );
			default:  return $this->_error( 'math_unknown_error', $errbit );
			}
		}

		$this->hash = NULL;
		$hash = substr( $contents, 1, 32 );
		if ( !preg_match( "/^[a-f0-9]{32}$/", $hash ) ) {
			return $this->_error( 'math_unknown_error' );
		}
		
		if( !file_exists( "$wgTmpDirectory/{$hash}.png" ) ) {
			return $this->_error( 'math_image_error' );
		}
		
		$this->hash = $hash;
		$tmp = $this->moveToMathDir( "{$hash}.png" );
		if ( $tmp !== false ) {
			$this->hash = NULL;
			return $tmp;
		}

		return false;
	}

	/**
	 * Invoke the blahtex executable.
	 * This function invokes the @c blahtex helper program. The
	 * location of the program is specified in $wgBlahtex. Extra
	 * options may be specified in $wgBlahtexOptions.
	 * @param $tex String containing the LaTeX fragment to be rendered.
	 * @param $makePNG Boolean specifying whether blahtex should
	 * generate both MathML and PNG (@c true) or only MathML (@c false).
	 * @return A 2-tuple. 
	 *  - If an error occurred, then the first element is @c false and
	 *    the second element is a string containing an HTML fragment
	 *    with the error message.
	 *  - Otherwise, the first element is @c true and the second
	 *    element s a string containing the output of @c blahtex.
	 */
	function invokeBlahtex( $tex, $makePNG )
	{
		global $wgBlahtex, $wgBlahtexOptions, $wgTmpDirectory;

		$descriptorspec = array( 0 => array( "pipe", "r" ),
					 1 => array( "pipe", "w" ) );
		$options = '--mathml ' . $wgBlahtexOptions;
		if ( $makePNG ) 
			$options .= " --png --temp-directory $wgTmpDirectory --png-directory $wgTmpDirectory";

		$process = proc_open( $wgBlahtex.' '.$options, $descriptorspec, $pipes );
		if ( !$process ) {
			return array( false, $this->_error( 'math_unknown_error', ' #1' ) );
		}
		fwrite( $pipes[0], '\\displaystyle ' );
		fwrite( $pipes[0], $tex );
		fclose( $pipes[0] );
		
		$contents = '';
		while ( !feof($pipes[1] ) ) {
			$contents .= fgets( $pipes[1], 4096 );
		}
		fclose( $pipes[1] );
		if ( proc_close( $process ) != 0 ) {
			// exit code of blahtex is not zero; this shouldn't happen
			return array( false, $this->_error( 'math_unknown_error', ' #2' ) );
		}
		
		return array( true, $contents );
	}

	/**
	 * Process blahtex output.
	 * Parse the output and fill the mathml field in the database. If
	 * blahtex has also generated a PNG image, then update the hash
	 * field as well move the PNG image to its final destination. 
	 * @param $contents String containing blahtex output.
	 * @return String containing HTML fragment with error message if
	 * an error occurred, @c false otherwise.
	 */
	function processBlahtexOutput( $results )
	{
		if ( isset( $results["blahtex:logicError"] ) ) {
			// Something went completely wrong
			return $this->_error('math_unknown_error', $results["blahtex:logicError"]);

		} elseif ( isset( $results["blahtex:error:id"] ) ) {
			// There was a syntax error in the input
			return $this->blahtexError( $results, "blahtex:error" );

		} elseif (isset($results["mathmlMarkup"]) || isset($results["blahtex:png:md5"])) {
			// We got some results
			if ( isset( $results["mathmlMarkup"] ) )	 
				$this->mathml = $results['mathmlMarkup'];
			if ( isset( $results["blahtex:png:md5"] ) ) {
				$this->hash = $results["blahtex:png:md5"];
				$tmp = $this->moveToMathDir( "{$this->hash}.png" );
				if ( $tmp !== false ) 
					return $tmp;
			}
			return false;

		} else {
			// There is an error somewhere
			if ( isset( $results["blahtex:mathml:error:id"] ) ) 
				return $this->blahtexError( $results, "blahtex:mathml:error" );
			if ( isset( $results["blahtex:png:error:id"] ) )
				return $this->blahtexError( $results, "blahtex:png:error" );
			return $this->_error( 'math_unknown_error', ' #3' );
		}
	}

	/**
	 * Build an error message for blahtex.
	 * @param $results Parse tree as returned by
	 * blahtexOutputParser::parse() .
	 * @param $node String representing the node in the tree that the
	 * message is stored under.
	 * @returns String containing HTML fragment with the error
	 * message. 
	 */
	function blahtexError( $results, $node ) {
		$id = 'math_' . $results[$node . ":id"];
		$fallback = $results[$node . ":message"];
		if ( isset( $results[$node . ":arg"] ) ) {
			if ( is_array( $results[$node . ":arg"] ) ) {
				// Error message has two or three arguments
				$arg1 = $results[$node . ":arg"][0];
				$arg2 = $results[$node . ":arg"][1];
				if ( count( $results[$node . ":arg"][1] > 2 ) )
					$arg3 = $results[$node . ":arg"][1];
				else
					$arg3 = '';
				return $this->_error( $id, $arg1, $arg2, $arg3, $fallback );
			} else {
				// Error message has one argument
				$arg = $results[$node . ":arg"];
				return $this->_error( $id, $arg, '', '', $fallback );
			}
		}
		else {
			// Error message without arguments
			return $this->_error( $id, '', '', '', $fallback );
		}
	}
		
	/**
	 * Move a PNG image to its final destination.
	 * The file is moved from $wgTmpDirectory to a directory under
	 * $wgMathDirectory. This function assumes that $hash is set.
	 * @param $fname String containing name of file to be moved.
	 * @return String containing HTML fragment with error message if
	 * an error occurred, @c false otherwise.
	 */
	function moveToMathDir( $fname ) {
		global $wgTmpDirectory;

		$hashpath = $this->_getHashPath();
		if( !file_exists( $hashpath ) ) {
			if( !@wfMkdirParents( $hashpath, 0755 ) ) {
				return $this->_error( 'math_bad_output' );
			}
		} elseif( !is_dir( $hashpath ) || !is_writable( $hashpath ) ) {
			return $this->_error( 'math_bad_output' );
		}
		
		if( !rename( "$wgTmpDirectory/$fname", "$hashpath/$fname" ) ) {
			return $this->_error( 'math_output_error' );
		}
		return false;
	}

	/**
	 * Build an error message in HTML.
	 * @param $msg String containing lookup key for the message; will
	 * be passed on to wfMsg() .
	 * @param $arg1 String containing first argument for the message.
	 * @param $arg2 String containing second argument for the message.
	 * @param $arg3 String containing third argument for the message.
	 * @param $fallback String containing a fallback message in case
	 * the lookup key in $msg is not found.
	 * @return String containing HTML fragment with the error message.
	 */
	function _error( $msg, $arg1 = '', $arg2 = '', $arg3 = '', $fallback = NULL ) {
		$mf = htmlspecialchars( wfMsg( 'math_failure' ) );
		if ( $msg ) {
			if ( $fallback && wfMsg( $msg ) == '&lt;' . htmlspecialchars( $msg ) . '&gt;' ) 
				$errmsg = htmlspecialchars( $fallback );
			else
				$errmsg = htmlspecialchars( wfMsg( $msg, $arg1, $arg2, $arg3 ) );
		}
		else
			$errmsg = '';
		$source = htmlspecialchars( str_replace( "\n", ' ', $this->tex ) );
		// Note: the str_replace above is because the return value must not contain newlines
		return "<strong class='error'>$mf ($errmsg): $source</strong>\n";
	}

	/**
	 * Recall cached information from the database.
	 * This function computes the hash value for the formula specified
	 * in $tex and looks whether any information is stored in the @c
	 * math table in the database. In that case, the $hash,
	 * $conservativeness, $html and $mathml member variables are
	 * updated. 
	 * @return @c true if information was found in the database, @c
	 * false if not.
	 */
	function _recall() {
		global $wgMathDirectory;
		$fname = 'MathRenderer::_recall';

		$this->md5 = md5( $this->tex );
		$dbr =& wfGetDB( DB_SLAVE );
		$rpage = $dbr->selectRow( 'math', 
					  array( 'math_outputhash','math_html_conservativeness','math_html','math_mathml' ),
					  array( 'math_inputhash' => pack("H32", $this->md5)), # Binary packed, not hex
					  $fname
		);

		if( $rpage === false ) 
			return false; // Missing from the database

		if( $rpage->math_outputhash == '' )
			$this->hash = NULL;
		else {
			// Tailing 0x20s can get dropped by the database, add them back on if necessary:
			$xhash = unpack( 'H32md5', $rpage->math_outputhash . "                " );
			$this->hash = $xhash ['md5'];
		}
		
		$this->conservativeness = $rpage->math_html_conservativeness;
		$this->html = $rpage->math_html;
		$this->mathml = $rpage->math_mathml;
		
		if( !$this->html && !$this->mathml && !$this->hash )
			return false; // Database contains no useful information

		if( $this->hash) {
			$hashpath = $this->_getHashPath();

			// MediaWiki 1.5 / 1.6 transition:
			// All files used to be stored directly under $wgMathDirectory
			// Move them to the new layout if necessary
			if( file_exists( $wgMathDirectory . "/{$this->hash}.png" )
			    && !file_exists( $hashpath . "/{$this->hash}.png" ) ) {
				if( !file_exists( $hashpath ) ) {
					if( !@wfMkdirParents( $hashpath, 0755 ) ) {
						return false;
					}
				} elseif( !is_dir( $hashpath ) || !is_writable( $hashpath ) ) {
					return false;
				}
				if ( function_exists( "link" ) ) {
					return link ( $wgMathDirectory . "/{$this->hash}.png",
						      $hashpath . "/{$this->hash}.png" );
				} else {
					return rename ( $wgMathDirectory . "/{$this->hash}.png",
							$hashpath . "/{$this->hash}.png" );
				}
			}
			
			if ( !file_exists( $hashpath . "/{$this->hash}.png" ) ) {
				$this->hash = NULL; // File disappeared from the render cache
				return false;
			}
		}
		
		return true;
	}

	/**
	 * Do the actual rendering.
	 * After all preliminaries are completed, this function chooses
	 * between PNG, HTML, or MathML output depending on the output mode
	 * stored in $mode and the available options and returns a
	 * rendering of the specified formula.
	 * @return String containing HTML fragment representing the formula.
	 */
	function _doRender() {

		switch( $this->mode ) {

		case MW_MATH_PNG: 
			if( $this->hash )
				$choice = 'png';
			elseif ( $this->html )
				$choice = 'html';
			else
				$choice = 'mathml';
			break;

		case MW_MATH_SIMPLE:
			if( $this->hash && ( !$this->html || $this->conservativeness != 2 ) )
				$choice = 'png';
			elseif ( $this->html )
				$choice = 'html';
			else
				$choice = 'mathml';
			break;

		case MW_MATH_HTML:
			if ( $this->html )
				$choice = 'html';
			elseif( $this->hash )
				$choice = 'png';
			else
				$choice = 'mathml';
			break;

		case MW_MATH_MODERN:
			if( $this->hash && ( !$this->html || $this->conservativeness == 0 ) )
				$choice = 'png';
			elseif ( $this->html )
				$choice = 'html';
			else
				$choice = 'mathml';
			break;

		case MW_MATH_MATHML:
			if ( $this->mathml )
				$choice = 'mathml';
			elseif( $this->hash )
				$choice = 'png';
			else
				$choice = 'html';
			break;

		}

		if( $choice == 'mathml' )
			return "<math xmlns='http://www.w3.org/1998/Math/MathML'>{$this->mathml}</math>";
		elseif( $choice == 'png' )
			return $this->_linkToMathImage();
		else
			return '<span class="texhtml">'.$this->html.'</span>';
	}

	/**
	 * Construct a link to PNG file 
	 * @return String containing HTML fragment with the PNG file
	 * representing the formula. 
	 */
	function _linkToMathImage() {
		global $wgMathPath;
		$url = htmlspecialchars( "$wgMathPath/" . substr($this->hash, 0, 1)
					.'/'. substr($this->hash, 1, 1) .'/'. substr($this->hash, 2, 1)
					. "/{$this->hash}.png" );
		$alt = trim(str_replace("\n", ' ', htmlspecialchars( $this->tex )));
		return "<img class='tex' src=\"$url\" alt=\"$alt\" />";
	}

	/**
	 * Get directory to store PNG image in.
	 * The PNG images are stored in a tiered directory tree under
	 * $wgMathDirectory. This function compute the directory that the
	 * PNG image for the specified formula should go in.
	 * @return String with directory path.
	 */
	function _getHashPath() {
		global $wgMathDirectory;
		$path = $wgMathDirectory .'/'. substr($this->hash, 0, 1)
					.'/'. substr($this->hash, 1, 1)
					.'/'. substr($this->hash, 2, 1);
		wfDebug( "TeX: getHashPath, hash is: $this->hash, path is: $path\n" );
		return $path;
	}

}

/**
 * Render a LaTeX fragment.
 * @param $tex String containing the LaTeX fragment.
 * @return String containing an HTML fragment representing the formula
 * specified in $tex.
 */
function renderMath( $tex ) {
	global $wgUser;
	$math = new MathRenderer( $tex );
	$math->setOutputMode( $wgUser->getOption( 'math' ) );
	return $math->render();
}

?>
