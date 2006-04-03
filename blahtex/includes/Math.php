<?php
/**
 * Contain everything related to <math> </math> parsing
 * @package MediaWiki
 */

class blahtexOutputParser  {
	var $parser;
	var $stack;
	var $results;

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
 * Takes LaTeX fragments, sends them to a helper program (texvc) for rendering
 * to rasterized PNG and HTML and MathML approximations. An appropriate
 * rendering form is picked and returned.
 * 
 * by Tomasz Wegrzanowski, with additions by Brion Vibber (2003, 2004)
 *
 * @package MediaWiki
 */
class MathRenderer {
	var $mode = MW_MATH_MODERN;
	var $tex = '';
	var $inputhash = '';
	var $hash = '';
	var $html = '';
	var $mathml = '';
	var $conservativeness = 0;
	
	function MathRenderer( $tex ) {
		$this->tex = $tex;
	 }
	
	function setOutputMode( $mode ) {
		$this->mode = $mode;
	}

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
	 * Test whether the necessary directories and executables exists.
	 * Returns an error message if there is a problem, and false otherwise.
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
	 * If there is an error, the return value is (false, error message).
	 * If there is no error, the return value is (true, texvc output).
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
	 * Process texvc output: fill the mathml, html, hash, and conservativeness fields
	 * in the database and move the PNG image to its final destination.
	 * Returns an error message, or false if no error occurred.
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
	 * If there is an error, the return value is (false, error message).
	 * If there is no error, the return value is (true, blatex output).
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
	 * Process blahtex output and update the mathml and png fields.
	 * Returns an error message, or false if no error occurred.
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
	 * Returns the error message stored in the $results parse tree
	 * under the node $node.
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
	 * Move a file from $wgTmpDirectory to a directory under $wgMathDirectory.
	 * Assumes that $this->hash is set.
	 * Returns false or an error message.
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
	 * Select among PNG, HTML, or MathML output depending on the user's preference
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

	function _linkToMathImage() {
		global $wgMathPath;
		$url = htmlspecialchars( "$wgMathPath/" . substr($this->hash, 0, 1)
					.'/'. substr($this->hash, 1, 1) .'/'. substr($this->hash, 2, 1)
					. "/{$this->hash}.png" );
		$alt = trim(str_replace("\n", ' ', htmlspecialchars( $this->tex )));
		return "<img class='tex' src=\"$url\" alt=\"$alt\" />";
	}

	function _getHashPath() {
		global $wgMathDirectory;
		$path = $wgMathDirectory .'/'. substr($this->hash, 0, 1)
					.'/'. substr($this->hash, 1, 1)
					.'/'. substr($this->hash, 2, 1);
		wfDebug( "TeX: getHashPath, hash is: $this->hash, path is: $path\n" );
		return $path;
	}

}

function renderMath( $tex ) {
	global $wgUser;
	$math = new MathRenderer( $tex );
	$math->setOutputMode( $wgUser->getOption( 'math' ) );
	return $math->render();
}

?>
