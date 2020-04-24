/* File: lcdbezel.scad
 * 
 * Author: deh
 * Latest edit: 20200423
 */
 
 $fn = 40;
 
 /* Reference: origin to bottom-left corner of pcb */
 
 /* LCD module */
 brdlen   = 98.2;   // Overall pcb length
 brdwid   = 60;     // Overall pcb width
 brdholeoffset = 2.1; // Mounting holes from pcb edge
 brdholedia    = 3.5; // Mounting hole diameter
 brdthick = 1.7;    // pcb thickness
 
 dspoff_x = 1.0;    // Display offset x
 dspoff_y = 10.4;   // Display offset y
 dsplen   = 97.1;   // Display length
 dspwid   = 39.7;   // Display width
 dspthick = 10.0;   // Display thickness
 dspdepth = 13;     // Top of pcb to floor below
 
 conlen   = 16;     // Connector w cable length (x)
 conwid   = 10.6;   // Connector width
 conoff_y = 45;     // Connector offset (y)
 
 /* Bottom box */
 bbxwall  = 3;     // Thickness of walls
 bbxflr   = 2;     // Thickness of floor
 bbxrad   = 3;     // Radius of outside corners
 bbxht    = 15;    // Height (outside)
 
 /* Pin protrusion spacer. */
 ppsthick = 3;     // Thickness of space
 
 /* Top bezel */
 tbzthick = dspthick - ppsthick;

/* Bottom box */ 
bbxlen = brdlen + conlen + 2*bbxwall;
bbxwid = brdwid + 2*bbxwall;
bbxoff_x = -conlen;
bbxoff_y = -bbxwall;

module rounded_bar(d, l, h)
{
    // Rounded end
    cylinder(d = d, h = h, center = false);
    // Bar
    translate([0, -d/2, 0])
       cube([l, d, h],false);
}
module eye_bar(d1, d2, len, ht)
{
   difference()
   {   
      rounded_bar(d1,len,ht);
      cylinder(d = d2, h = ht + .001, center = false);
   }
}
psto_dia = 7;   // Post outside diameter
psto_hole = 3.0; // Post hole diameter
module post_outside(a,z,holedia)
{
    translate(a)
    {
        difference()
        {
            union()
            {
                eye_bar(psto_dia,holedia,psto_dia*.5,z);
                translate([0,psto_dia*.5,0])
                
                cube([psto_dia*.5,psto_dia*.5,z],center=false);
            }
            union()
                translate([0,psto_dia,0])
                cylinder(d = psto_dia,h = z+.01, center=false);
            {
            }
        }
    }
}
/* bframe: basic outline with mounting tabs */
// zht: height bottom to top
module bframe(zht,holedia)
{
    translate([bbxoff_x,bbxoff_y,0])
    {
        difference()
        {
            union()
            {
                cube([bbxlen,bbxwid,zht-.01],center=false);
                
                // Four corner posts
                translate([bbxwall+bbxlen,bbxwid+bbxwall-psto_dia+.5,0])
                  rotate([0,0,180])
                    post_outside([0,0,0],zht,holedia);
                
                translate([bbxlen+bbxwall,bbxwall+.5,0])
                   mirror([1,0,0])
                    post_outside([0,0,0],zht,holedia);

                translate([-bbxwall,bbxwid-psto_dia*.5,0])
                   mirror([1,0,0])
                    rotate([0,0,180])
                    post_outside([0,0,0],zht,holedia);
                
                post_outside([-bbxwall,psto_dia*.5,0],zht,holedia);
            }
            union()
            {
                translate([bbxwall,bbxwall,bbxflr])
                    cube([bbxlen-2*bbxwall,bbxwid-2*bbxwall,zht-bbxflr],center=false);
            }
        }
    }
}

bbx(6);
