/* File: ftdi_clip.scad
 * Clip for 2nd FTDI 
 * Author: deh
 * Latest edit: 20191002
 */

 $fn = 20;
 
 thick = 2.0;

/* FTDI unit goes between pcb and case wall */
ftd_base_len = 35;	// Lenth of base part
ftd_post_len = 15;	// Length of tall part
ftd_slot_base = thick;// Offset from bottom of case
ftd_slot_wid = 2;		// Width of pcb

ftd_slot_wid2 = 3.0;	// Width at ahead of IC
ftd_slot_wid3 = 3.9;	// Width at IC
ftd_in_wid = 2.0;	// Inner wall width
ftd_tot_wid = ftd_slot_wid2+ftd_in_wid+1.5;	// Width total
ftd_tot_len = 20;
ftd_ic_len = 12;	// IC length
ftd_tot_ht = 18.6;

/* ***** fillet *****
 * r = radius of fillet
 * l = length of fillet
 fillet (r,l);
*/
module fillet(r,l)
{
   difference()
   {
     union()
     {
       cube([2*r,2*r,l]);
     }
     union()
     {
       translate ([r,r,0])
           cylinder(d = 2*r + 0.1, h = l, center = false);
       translate([r,0,0])
           cube([r,2*r,l],center = false);
       translate([0,r,0])
           cube([2*r,r,l],center = false);
     }
   }
}
module ftdi_post()
{
echo(ftd_tot_wid,"ftd_tot_wid");
	translate([0,0,thick])
	{
		difference()
		{
			union()
			{
				translate([0,0,0])
					cube([ftd_tot_wid+2, ftd_tot_len, ftd_tot_ht],center=false);

/*				translate([0.05,ftd_tot_len,-.05])
					rotate([0,-90,0])
					rotate([90.0,0])
			 			fillet (4,ftd_tot_len);	
*/
			}
			union()
			{
              translate([1,0,0])
              {  
				translate([ftd_in_wid,0,1])
					cube([ftd_slot_wid2,ftd_tot_len,ftd_tot_ht],center=false);
				ftx = ftd_in_wid - (ftd_slot_wid3 - ftd_slot_wid2);
				fty = ftd_tot_len - ftd_ic_len;
				translate([ftx,fty,4])
					cube([ftd_slot_wid3,ftd_ic_len,ftd_tot_ht],center=false);
              }
			
			}
	 	}
	}
}
module ftdi_base(a)
{
    translate([0,0,0])
        cube([30,20,2],center=false);
}

ftdi_post();
ftdi_base([0,0,0]);
 
