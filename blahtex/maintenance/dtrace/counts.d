/*
 * This software is in the public domain.
 *
 * $Id: counts.d,v 1.1 2006/03/23 10:12:47 jitseniesen Exp $
 */

#pragma D option quiet

self int tottime;
BEGIN {
	tottime = timestamp;
}

php$target:::function-entry
	@counts[copyinstr(arg0)] = count();
}

END {
	printf("Total time: %dus\n", (timestamp - tottime) / 1000);
	printf("# calls by function:\n");
	printa("%-40s %@d\n", @counts);
}

