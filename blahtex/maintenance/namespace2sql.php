<?php
# $Header: /home/xubuntu/berlios_backup/github/tmp-cvs/blahtex/Repository/blahtex/maintenance/namespace2sql.php,v 1.1 2005/08/22 17:15:51 jitseniesen Exp $
#
# Print SQL to insert namespace names into database.
# This source code is in the public domain.

require_once( "commandLine.inc" );

for ($i = -2; $i < 16; ++$i) {
	$nsname = wfStrencode( $wgLang->getNsText( $i ) );
	$dbname = wfStrencode( $wgDBname );
	print "INSERT INTO ns_name(ns_db, ns_num, ns_name) VALUES('$dbname', $i, '$nsname');\n";
}

?>
