/* File: pot_mnt.scad -- Stacked
 * Clip for pot mount for Control Panel simulator
 * Author: deh
 * Latest edit: 20191002
 */

include <../../../../git_3D/scad/library_deh/deh_shapes.scad>

 $fn = 40;
 
 thick = 2.0;
 
 /* ***** wedge *****
 * l = length
 * w = width
 * h = height/thickness
 * wedge(l, w, h)
*/
 module wedge(a,angle)
 {
g = angle;
w = 40;
h = w*sin(g);     
l = w*cos(g); 
     translate(a)
     {
       rotate([0,0,90])
       rotate([90,0,0])
       linear_extrude(height=20,center=true)
        polygon(points=[[0,0],[l,h],[-l,h]]);
     }
 }
 
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
stopht=25;    
ofs = 18;    
    translate(a)
    {
        difference()
        {
            union()
            {
                cube([50,40,2],center=false);
                translate([0,20-ofs,0])
                    cube([21,3,stopht],center=false);
                translate([0,20-3+ofs,0])
                    cube([21,3,stopht],center=false);
            }
            union()
            {
                wedge([9,20,20],10);

            }
        }
    }
}
module lever(a)
{
    translate(a)
    {
        rotate([0,0,0])
        {
         difference()
         {
            union()
            {
                cylinder(d=12,h=6,center=false);
            }
            union()
            {
                cylinder(d=6.5,h=50,center=true);
            }
         }
         translate([1.5,-3,0])
            cube([2.5,6,6],center=false);
       }
       translate([4,-2,0])
          cube([50,4,6],center=false);

    }
}

rotate([0,90,0])
    pot_mnt([-20,0,0]);

plat([-20,-20,0]);

lever([50,0]);
