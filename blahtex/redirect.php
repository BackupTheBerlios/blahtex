<?php
unset( $DP );
unset( $IP );
$wgCommandLineMode = false;
define( 'MEDIAWIKI', true );

require_once( './includes/Defines.php' );
require_once( './LocalSettings.php' );
global $wgArticlePath;

require_once( 'includes/WebRequest.php' );
$wgRequest = new WebRequest();

$page = $wgRequest->getVal( 'wpDropdown' );

$url = str_replace( "$1", urlencode( $page ), $wgArticlePath );

header( "Location: {$url}" );
?>
