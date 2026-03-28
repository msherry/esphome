// ============================================================
//  Parametric electronics enclosure
//  Units: mm
//  Faces:
//    Top (lid)   — screen cutout
//    Bottom      — ethernet cutout
//    Right (X+)  — USB-C cutout
// ============================================================

// --- Box outer dimensions ---
box_l = 101.6;   // length X  (4.00 in)
box_w = 57.15;   // width  Y  (2.25 in)
box_h = 25.4;    // height Z  (1.00 in)

// --- Wall & lid ---
wall    = 2.0;   // wall thickness
lid_h   = 2.5;   // lid plate thickness
lid_lip = 1.8;   // how far the alignment lip drops into the box
tol     = 0.25;  // fit tolerance between lip and box opening

// --- Cutouts ---
// Screen (top face / lid)
screen_w = 25.41;
screen_h = 17.02;

// Ethernet (bottom face)
eth_w = 16.85;
eth_h = 16.10;

// USB-C (right face, X+)
// Standard USB-C receptacle ~8.94 × 2.56 mm; add margin for shell + alignment
usbc_w  = 9.5;   // Y span
usbc_h  = 3.5;   // Z span

// --- Screw bosses (M3) ---
boss_od     = 6.0;   // boss outer diameter
boss_id     = 3.4;   // through-hole for M3 screw (clearance)
boss_thread = 3.2;   // tapped-style hole in boss base (no insert needed for 2-3 prints)
boss_inset  = wall + boss_od / 2 + 1.0;  // distance from outer wall to boss centre

$fn = 48;

// ============================================================
//  BOX BODY
// ============================================================
module box_body() {
    difference() {
        // Outer shell
        cube([box_l, box_w, box_h]);

        // Interior cavity (open top)
        translate([wall, wall, wall])
            cube([box_l - 2*wall,
                  box_w - 2*wall,
                  box_h - wall + 1]);   // +1 ensures clean open top

        // ---- Ethernet cutout — bottom face, centred ----
        translate([(wall + 1.7  ),
                   (box_w - eth_h) / 2,
                   -0.01])
            cube([eth_w, eth_h, wall + 0.02]);

        // ---- USB-C cutout — right face (X+), centred in Y and Z ----
        translate([box_l - wall - 0.01,
                   (box_w - usbc_w) / 2,
                   2.5])
            cube([wall + 0.02, usbc_w, usbc_h]);
    }

    // ---- M3 screw bosses at inner corners ----
    boss_height = box_h - wall - lid_h - 0.4;  // stops just below lid seating surface
    for (x = [boss_inset, box_l - boss_inset])
        for (y = [boss_inset, box_w - boss_inset])
            translate([x, y, wall])
                difference() {
                    cylinder(d = boss_od, h = boss_height);
                    // Blind hole — narrower than clearance to grip M3 directly
                    translate([0, 0, -0.01])
                        cylinder(d = boss_thread, h = boss_height + 0.02);
                }
}

// ============================================================
//  LID
// ============================================================

module lid() {
    difference() {
        union() {
            // Main plate
            cube([box_l, box_w, lid_h]);

            // Alignment lip — fits inside box opening with tol clearance
            lip_x = box_l - 2*wall - 2*tol;
            lip_y = box_w - 2*wall - 2*tol;
            translate([wall + tol, wall + tol, -lid_lip])
                cube([lip_x, lip_y, lid_lip]);
        }

        // ---- Screen cutout — centred on lid top face ----
        translate([(box_l - screen_w) / 2,
                   (box_w - screen_h) / 2,
                   -lid_lip -0.01])
            cube([screen_w, screen_h, lid_lip + lid_h + 0.02]);

        // ---- M3 clearance holes through lid (align with bosses) ----
        for (x = [boss_inset, box_l - boss_inset])
            for (y = [boss_inset, box_w - boss_inset])
                translate([x, y, -lid_lip -0.01])
                    cylinder(d = boss_id, h = lid_h + lid_lip + 0.02);
    }
}

// ============================================================
//  RENDER — parts side by side for slicing
//  Comment out one module to print separately.
// ============================================================
box_body();

translate([0, box_w + 15, 0])
    lid();

// ============================================================
//  NOTES
// ============================================================
//
//  Printing tips:
//    - Print box body upright (open face up), lid face-down
//    - 0.2 mm layer height, 3 perimeters, 20% infill
//    - No supports needed for either part
//
//  Hardware:
//    - 4× M3×8 or M3×10 machine screws
//    - Bosses are sized to self-tap for PLA/PETG (2-3 prints)
//    - For repeated assembly: press-fit M3 heat-set inserts into bosses
//      and open boss_thread to 4.5 mm (M3 insert OD)
//
//  Tolerances:
//    - Increase `tol` if lid lip fits too tight (try 0.3)
//    - Decrease `tol` if lid is loose (try 0.15)
//    - Ethernet / screen cutouts have no tolerance built in —
//      add 0.1–0.2 mm to each dimension if your module has a bezel
//
//  Bottom-face ethernet note:
//    - If you want ethernet on the rear face instead of the floor,
//      change the eth cutout translate to:
//        translate([(box_l - eth_w)/2, box_w - wall - 0.01, (box_h - eth_h)/2])
//          cube([eth_w, wall + 0.02, eth_h]);
// ============================================================
