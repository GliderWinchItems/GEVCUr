/* File: pot_mnt.scad -- Stacked
 * Clip for pot mount for Control Panel simulator
 * Author: deh
 * Latest edit: 20191002
 */

include <../../../../git_3D/scad/library_deh/deh_shapes.scad>

 $fn = 40;
 
 thick = 2.0;
 
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

module pot_mnt (a)
{
    offset = 9-1.5;    

	translate(a)
		/* ***** eyebar *****
 * rounded bar with hole in rounded end
 * module eye_bar(d1, d2, len, ht)
d1 = outside diameter of rounded end, and width of bar
d2 = diameter of hole in end of bar
*/  
    difference()
    {
        union()
        {
            eye_bar(25, 7.5, 20, 2);
            
            translate([18,12.5,2])
            rotate([90,-90,0])
            fillet(4,25);
        }
        union()
        {
            translate([0,offset,0])
                cube([3.5,2.0,20],center=true);
            translate([0,-offset,0])
                cube([3.5,2.0,20],center=true);

        }
    }
}

module plat(a)
{
    translate(a)
        cube([30,40,2],center=false);
}

rotate([0,90,0])
    pot_mnt([-20,0,0]);

plat([0,-20,0]);
