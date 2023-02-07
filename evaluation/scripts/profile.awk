#!/bin/awk -f

{
	if ($1 == "H2D") {
		h2d += $2;
		h2d_n++;
	} else if ($1 == "KRNL") {
		krnl += $2;
		krnl_n++;
	} else if ($1 == "D2H") {
		d2h += $2;
		d2h_n++;
	}
}

END {
	if (h2d_n > 0) {
		av = h2d/h2d_n;
		print "H2D: "av;
	}
	if (krnl_n > 0) {
		av = krnl/krnl_n;
		print "KRNL: "av;
	}
	if (d2h_n > 0) {
		av = d2h/d2h_n;
		print "D2H: "av;
	}
}
