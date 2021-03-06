// Hyperbolic Rogue -- the 'experiments with geometry' menu
// Copyright (C) 2011-2018 Zeno Rogue, see 'hyper.cpp' for details

// -- geometry menu --

namespace hr {

int eupage = 0;
int euperpage = 21;

string euchelp =
  "If you want to know how much the gameplay is affected by the "
  "hyperbolic geometry in HyperRogue, this mode is for you!\n\n"
  
  "You can try many different geometries here. We start by gluing "
  "n-gons in such a way that k of them meet in every vertex. "
  "Depending on n and k, this either folds into a sphere, unfolds into a plane, "
  "or requires a hyperbolic space. The result may be then 'bitrunc' by "
  "replacing each vertex by a 2k-gon. Furthermore, you can play "
  "with quotient geometries. For example, the elliptic geometry is "
  "obtained from the sphere by making the antipodes be the same point, "
  "so you return to the same spot (but as a mirror image) after going there. "
  "Have fun experimenting! "
  "Achievements and leaderboards do not work in geometry experiments, "
  "except some specific ones.\n\n"
  "In standard geometry (bitrunc or not), you can play the full game, but in other geometries "
  "you select a particular land. Lands are unlocked by visiting them in this "
  "session, or permanently by collecting 25 treasure. Try Crossroads in Euclidean "
  "or chaos mode in non-standard non-quotient hyperbolic to visit many lands. "
  "Highlights:\n"
  "* Crystal World and Warped Coast can be understood as extra geometries.\n"
  "* Halloween is specially designed for spherical geometry.\n"
  "* To see the difference, try Hunting Grounds in Euclidean -- it is impossible.\n";

int ewhichscreen = 2;

void showQuotientConfig() {
  using namespace fieldpattern;
  gamescreen(2);
  dialog::init(XLAT("advanced configuration"));
  fgeomextra& gxcur = fgeomextras[current_extra];
  for(int i=0; i<isize(fgeomextras); i++) {
    auto& g = fgeomextras[i];
    dialog::addBoolItem(XLAT(ginf[g.base].name), g.base == gxcur.base, 'a'+i);
    }
  nextPrimes(gxcur);
  for(int i=0; i<isize(gxcur.primes); i++) {
    auto& p = gxcur.primes[i];
    dialog::addBoolItem(XLAT("order %1%2 (non-bitruncated cells: %3)", its(p.p), p.squared ? "²" : "", its(p.cells)), i == gxcur.current_prime_id, 'A'+i);
    }
  
  if(isize(gxcur.primes) < 6) {
    dialog::addBreak(100);
    dialog::addHelp(
      "This geometry is obtained by applying the same 'generators' which "
      "lead to creating the given basic hyperbolic geometry, "
      "but using a fixed finite field instead of the field of reals. "
      "It can be also interpreted as a quotient of the given basic geometry. "
      "Warning: field patterns based on large primes might generate for a long time."
      );
    dialog::addBreak(100);
    }

  dialog::addItem("find the next prime", 'p');
  dialog::addItem("activate", 'x');
  dialog::addItem("default", 'c');

  keyhandler = [&gxcur] (int sym, int uni) {
    if(uni >= 'a' && uni < 'a' + isize(fgeomextras))
      current_extra = uni - 'a';
    else if(uni >= 'A' && uni < 'A' + isize(gxcur.primes))
      gxcur.current_prime_id = uni - 'A';
    else if(uni == 'p')
      nextPrime(gxcur);
    else if(uni == 'x' || uni == '\n') {
      targetgeometry = gxcur.base; stop_game_and_switch_mode(rg::geometry);
      enableFieldChange();
      targetgeometry = gFieldQuotient; stop_game_and_switch_mode(rg::geometry);
      start_game();
      }
    else if(uni == 'c') {
      targetgeometry = gEuclid; stop_game_and_switch_mode(rg::geometry);
      fieldpattern::quotient_field_changed = false;
      targetgeometry = gFieldQuotient; stop_game_and_switch_mode(rg::geometry);
      start_game();
      }
    else if(doexiton(sym, uni))
      popScreen();
    };
  
  dialog::display();
  }

bool torus_bitrunc;

void showTorusConfig() {
  cmode = sm::SIDE | sm::TORUSCONFIG;
  gamescreen(2);
  
  dialog::init(XLAT("advanced configuration"));
  
  auto& mode = torusconfig::tmodes[torusconfig::newmode];
  
  dialog::addSelItem(XLAT("mode"), XLAT(mode.name), 'm');
  
  bool single = (mode.flags & torusconfig::TF_SINGLE);
  bool square = (mode.flags & torusconfig::TF_SQUARE);
  bool simple = (mode.flags & torusconfig::TF_SIMPLE);
  
  if(single) {
    dialog::addSelItem(XLAT("number of cells (n)"), its(torusconfig::newqty), 'n');
    if(torusconfig::TF_HEX)
      dialog::addSelItem(XLAT("cell bottom-right from 0 (d)"), its(torusconfig::newdy), 'd');
    else
      dialog::addSelItem(XLAT("cell below 0 (d)"), its(torusconfig::newdy), 'd');
    }
  else {
    if(torusconfig::newsdx < 1) torusconfig::newsdx = 1;
    if(torusconfig::newsdy < 1) torusconfig::newsdy = 1;
    dialog::addSelItem(XLAT("width (x)"), its(torusconfig::newsdx), 'x');
    dialog::addSelItem(XLAT("height (y)"), its(torusconfig::newsdy), 'y');
    }

  if(square) dialog::addBoolItem(XLAT("bitruncated"), !torus_bitrunc, 't');
  else dialog::addInfo("", 100);
  
  int valid = 2;

  if(single) {  
    if(square) {
      dialog::addInfo("this mode has bad patterns", 0x808080), valid = 1;
      if(!torus_bitrunc && valid == 1)
        dialog::addInfo("incompatible with bitruncating", 0x808080), valid = 0;
      }
    else {
      if(torusconfig::newqty % 3)
        dialog::addInfo(XLAT("best if %1 is divisible by %2", "n", "3"), 0x808080), valid = 1;
      if((torusconfig::newdy + 999999) % 3 != 2)
        dialog::addInfo(XLAT("best if %1 is divisible by %2", "d+1", "3"), 0x808080), valid = 1;
      }
    }
  else {
    if(square) {
      if(torusconfig::newsdx & 1)
        dialog::addInfo(XLAT("best if %1 is divisible by %2", "x", "2"), 0x808080), valid = 1;
      if(torusconfig::newsdy & 1)
        dialog::addInfo(XLAT("best if %1 is divisible by %2", "y", "2"), 0x808080), valid = 1;
      if(!torus_bitrunc && valid == 1)
        dialog::addInfo("incompatible with bitruncating", 0x808080), valid = 0;
      }
    else if(simple) {
      if(torusconfig::newsdx & 1)
        dialog::addInfo(XLAT("best if %1 is divisible by %2", "x", "3"), 0x808080), valid = 1;
      if(torusconfig::newsdy & 1)
        dialog::addInfo(XLAT("best if %1 is divisible by %2", "y", "3"), 0x808080), valid = 1;
      }
    else {
      if(torusconfig::newsdx & 1)
        dialog::addInfo(XLAT("best if %1 is divisible by %2", "x", "3"), 0x808080), valid = 1;
      if(torusconfig::newsdy & 1)
        dialog::addInfo(XLAT("best if %1 is divisible by %2", "y", "2"), 0x808080), valid = 0;
      }
    }
  
  dialog::addSelItem(XLAT("scale factor"), fts(vid.scale), 'z');

#if CAP_RUG
  dialog::addBoolItem(XLAT("hypersian rug mode"), (rug::rugged), 'u');
#endif

  dialog::addItem("activate", 'a');
  dialog::addItem("default", 'c');

  keyhandler = [=] (int sym, int uni) {
    if(uni == 'm') {
      int i = torusconfig::newmode + 1;
      if(i >= isize(torusconfig::tmodes)) i = 0;
      torusconfig::newmode = torusconfig::eTorusMode(i);
      }
    else if(uni == 'n' && single)
      dialog::editNumber(torusconfig::newqty, 0, 1000, 3, torusconfig::def_qty, XLAT("number of cells (n)"), "");
    else if(uni == 'd' && single)
      dialog::editNumber(torusconfig::newdy, -1000, 1000, 3, -torusconfig::def_dy, XLAT("cell below 0 (d)"), "");
    else if(uni == 'x' && !single)
      dialog::editNumber(torusconfig::newsdx, 0, 1000, square ? 2 : 3, 12, XLAT("width (x)"), "");
    else if(uni == 'y' && !single)
      dialog::editNumber(torusconfig::newsdy, 0, 1000, square ? 2 : simple ? 3 : 2, 12, XLAT("height (y)"), "");
    else if(uni == 't')
      torus_bitrunc = !torus_bitrunc;
    else if((uni == 'a' || uni == '\n') && torusconfig::newqty >= 3 && valid) {
      targetgeometry = gNormal; stop_game_and_switch_mode(rg::geometry);
      torusconfig::torus_mode = torusconfig::newmode;
      torusconfig::qty = torusconfig::newqty;
      torusconfig::dy = torusconfig::newdy;
      torusconfig::sdx = torusconfig::newsdx;
      torusconfig::sdy = torusconfig::newsdy;
      torusconfig::activate();
      if((square && torus_bitrunc) != nonbitrunc) stop_game_and_switch_mode(rg::bitrunc);
      targetgeometry = gTorus; stop_game_and_switch_mode(rg::geometry);
      start_game();
      }
    else if(uni == 'c') {
      targetgeometry = gEuclid; stop_game_and_switch_mode(rg::geometry);
      torusconfig::torus_mode = torusconfig::tmSingle;
      torusconfig::qty = torusconfig::def_qty;
      torusconfig::dy = torusconfig::def_dy;
      targetgeometry = gTorus; stop_game_and_switch_mode(rg::geometry);
      start_game();
      }
    else if(uni == 'z') editScale();
#if CAP_RUG
    else if(uni == 'u') rug::select();
#endif
    else if(doexiton(sym, uni))
      popScreen();
    };
  
  dialog::display();
  }

string bitruncnames[2] = {" (b)", " (n)"};

void validity_info() {
  int vccolors[4] = {0xFF0000, 0xFF8000, 0xFFFF00, 0x00FF00};
  auto lv = land_validity(specialland);
  if(lv.flags & lv::display_error_message)
    dialog::addInfo(XLAT(lv.msg), vccolors[lv.quality_level]);
  else
    dialog::addBreak(100);
  }

bool showquotients;

void showEuclideanMenu() {
  cmode = sm::SIDE;
  gamescreen(0);  
  if(cheater) for(int i=0; i<landtypes; i++) landvisited[i] = true;
  for(int i=0; i<landtypes; i++)
    if(hiitemsMax(treasureType(eLand(i))) >= 25) landvisited[i] = true;
  landvisited[laCrossroads] = true;
  landvisited[laCrossroads4] = true;
  landvisited[laIce] = true;
  landvisited[laHunting] = true;
  landvisited[laMirrorOld] = true;
  landvisited[laPrincessQuest] = cheater || princess::everSaved;
  landvisited[laWildWest] = true;
  landvisited[laHalloween] = true;
  landvisited[laWarpCoast] = true;
  landvisited[laGraveyard] = true;
  landvisited[laDual] = true;
  landvisited[laDocks] |= landvisited[laWarpCoast];
  landvisited[laSnakeNest] |= landvisited[laRedRock];
  landvisited[laCA] = true;
  // for(int i=2; i<lt; i++) landvisited[i] = true;

  string validclasses[4] = {" (X)", " (½)", "", " (!)"};
  
  if(ewhichscreen == 2) {
    dialog::init(XLAT("experiment with geometry"));
    int ts = ginf[geometry].sides;
    int tv = ginf[geometry].vertex;
    int tq = ginf[geometry].quotientstyle;
    int nom = (nonbitrunc ? tv : tv+ts) * ((tq & qNONORIENTABLE) ? 2 : 4);
    int denom = (2*ts + 2*tv - ts * tv);
    
    if(gp::on) {
      denom *= 2;
      nom = nom / tv * (2*tv + ts * (gp::area-1));
      if(nom % 2 == 0) nom /= 2, denom /= 2;
      }
    
    dialog::addSelItem(XLAT("land"), XLAT1(linf[specialland].name), '5');
    dialog::addBreak(50);

    for(int i=0; i<gGUARD; i++) {
      bool on = geometry == i;
      dynamicval<eGeometry> cg(geometry, eGeometry(i));
      if(!!(quotient || elliptic || torus) != showquotients) continue;
      dialog::addBoolItem(XLAT(ginf[i].name), on, 'a'+i);
      dialog::lastItem().value += validclasses[land_validity(specialland).quality_level];
      }
    
    dialog::addBreak(50);
    dialog::addBoolItem(XLAT("show quotient spaces"), showquotients, 'u');
    dialog::addBreak(50);
  
    if(ts == 6 && tv == 3)
      dialog::addSelItem(XLAT("bitruncated"), XLAT("does not matter"), 't');
    else if(S3 != 3)
      dialog::addBoolItem(XLAT("bitruncated"), !nonbitrunc, 't');
    else {
      dialog::addBoolItem(XLAT("Goldberg"), nonbitrunc, 't');
      dialog::lastItem().value = gp::operation_name();
      }
    
  
    dialog::addBreak(25);
    validity_info();    
    dialog::addBreak(25);
  
    int worldsize;
    
    int gar = 
      gp::on ? gp::area - 1 :
      nonbitrunc ? 0 :
      2;    
    
    switch(geometry) {
      case gTorus: 
        worldsize = torusconfig::qty;
        break;
      
      case gZebraQuotient:
        worldsize = 12 + 14 * gar;
        break;
      
      case gFieldQuotient:
        worldsize = isize(currfp.matrices) / ts;
        worldsize = worldsize * (2*tv + ts * gar) / tv / 2;
        break;
      
      case gKleinQuartic:
        worldsize = 24 + 28 * gar;
        break;
      
      case gBolza:
        worldsize = 3 * (2*tv + ts * gar) / tv;
        break;
      
      case gBolza2:
        worldsize = 6 * (2*tv + ts * gar) / tv;
        break;
      
      default:
        worldsize = denom ? nom/denom : 0;
        break;
      }
  
    dialog::addSelItem(XLAT("sides per face"), its(ts), 0);
    dialog::addSelItem(XLAT("faces per vertex"), its(tv), 0);
  
    string qstring = "none";
    if(tq & qZEBRA) qstring = "zebra";
  
    else if(tq & qFIELD) qstring = "field";
  
    else if((tq & qNONORIENTABLE) && sphere) qstring = "elliptic";
  
    else if(tq & qTORUS) qstring = "torus";
    
    else if(tq & qSMALL) qstring = ginf[geometry].shortname;
  
    dialog::addSelItem(XLAT("quotient space"), XLAT(qstring), 0);
  
    dialog::addSelItem(XLAT("size of the world"), 
      worldsize < 0 ? "exp(∞)*" + (nom%denom ? its(nom)+"/"+its(-denom) : its(-worldsize)): 
      worldsize == 0 ? "∞" :
      its(worldsize),
      '3');
    
    switch(ginf[geometry].cclass) {
      case 0:
        dialog::addSelItem(XLAT("Curvature"), XLAT("hyperbolic"), 0);
        break;
      
      case 1: 
        dialog::addSelItem(XLAT("Curvature"), XLAT("flat"), 0);
        break;
      
      case 2:
        dialog::addSelItem(XLAT("Curvature"), XLAT("spherical"), 0);
        break;
      }
    
    if(sphere) 
      dialog::addBoolItem(XLAT("stereographic/orthogonal"), vid.alpha>10, '1');
    else
      dialog::addBoolItem(XLAT("Poincaré/Klein"), vid.alpha>.5, '1');
    if(torus || geometry == gFieldQuotient)
      dialog::addItem(XLAT("advanced parameters"), '4');  
    dialog::addHelp();
    dialog::addBack();
    dialog::display();
  
    keyhandler = [] (int sym, int uni) {
      dialog::handleNavigation(sym, uni);
      if(uni >= 'a' && uni < 'a'+gGUARD) {
        targetgeometry = eGeometry(uni - 'a');
        stop_game_and_switch_mode(geometry == targetgeometry ? rg::nothing : rg::geometry);
        start_game();
        }
      else if(uni == 'u') 
        showquotients = !showquotients;
      else if(uni == 't') {
        if(euclid6) ;
        else if(S3 == 3) 
          gp::configure();
        else {
          stop_game_and_switch_mode(rg::bitrunc);
          start_game();
          }
        }
      else if(uni == '2' || sym == SDLK_F1) gotoHelp(euchelp);
      else if(uni == '3') { viewdists = !viewdists; if(viewdists) popScreenAll(); }
      else if(uni == '1' && !euclid) {
        if(sphere) {
          if(vid.alpha < 10) { vid.alpha = 999; vid.scale = 998; }
          else {vid.alpha = 1; vid.scale = .4; }
          }
        else {
          if(vid.alpha > .5) { vid.alpha = 0; vid.scale = 1; }
          else {vid.alpha = 1; vid.scale = 1; }
          }
        }
      else if(uni == '5')
        ewhichscreen ^= 3;
      else if(uni == '4') {
        if(torus) 
          torusconfig::newdy = torusconfig::dy,
          torusconfig::newqty = torusconfig::qty,
          torusconfig::newsdx = torusconfig::sdx,
          torusconfig::newsdy = torusconfig::sdy,
          torusconfig::newmode = torusconfig::torus_mode,
          torus_bitrunc = nonbitrunc,
          pushScreen(showTorusConfig);
        if(geometry == gFieldQuotient) 
          pushScreen(showQuotientConfig);
        }
      else if(doexiton(sym, uni))
        popScreen();
      };
    }
  else {
    dialog::init(XLAT("experiment with geometry"));
  
    dialog::addSelItem(XLAT("geometry"), XLAT(ginf[geometry].name) + XLAT(bitruncnames[nonbitrunc]), '5');
    dialog::addBreak(50);
    
    generateLandList([] (eLand l) { return land_validity(l).flags & lv::appears_in_geom_exp; });
    stable_sort(landlist.begin(), landlist.end(), [] (eLand l1, eLand l2) { return land_validity(l1).quality_level > land_validity(l2).quality_level; });
    
    for(int i=0; i<euperpage; i++) {
      if(euperpage * eupage + i >= isize(landlist)) { dialog::addBreak(100); break; }
      eLand l = landlist[euperpage * eupage + i];
      char ch;
      if(i < 26) ch = 'a' + i;
      else ch = 'A' + (i-26);
      string s = XLAT1(linf[l].name);

      if(landvisited[l]) {
        dialog::addBoolItem(s, l == specialland, ch);
        }
      else {
        dialog::addSelItem(s, XLAT("(locked)"), ch);
        }
      
      dialog::lastItem().color = linf[l].color;
      dialog::lastItem().value += validclasses[land_validity(l).quality_level];
      }
    dialog::addBreak(50);
    if(chaosUnlocked && !quotient && !euclid && !sphere)
      dialog::addItem(XLAT("Chaos mode"), '1');
    dialog::addItem(XLAT("next page"), '-');

    dialog::addBreak(25);
    validity_info();    
    dialog::addBreak(25);
    
    dialog::addHelp();
    dialog::addBack();
    dialog::display();
  
    keyhandler = [] (int sym, int uni) {
      dialog::handleNavigation(sym, uni);
      int lid;
      if(uni >= 'a' && uni <= 'z') lid = uni - 'a';
      else if(uni >= 'A' && uni <= 'Z') lid = 26 + uni - 'A';
      else lid = -1;
      
      if(lid >= 0) lid += euperpage * eupage;
      
      if(uni == '5') 
        ewhichscreen ^= 3;
      else if(uni == '-' || uni == PSEUDOKEY_WHEELUP || uni == PSEUDOKEY_WHEELDOWN) {
        eupage++;
        if(eupage * euperpage >= isize(landlist)) eupage = 0;
        }
      else if(uni == '1') {
        if(chaosUnlocked) {
          stop_game_and_switch_mode(rg::chaos);
          start_game();
          }
        }
      else if(lid >= 0 && lid < isize(landlist)) {
        eLand nland = landlist[lid];
        if(landvisited[nland]) {
          firstland = specialland = nland;
          stop_game_and_switch_mode(tactic::on ? rg::tactic : rg::nothing);
          start_game();
          }
        }
      else if(uni == '2' || sym == SDLK_F1) gotoHelp(euchelp);
      else if(doexiton(sym, uni)) popScreen();
      };
    }
  
  }

void runGeometryExperiments() {
  if(!geometry)
    specialland = getLandForList(cwt.c);
  pushScreen(showEuclideanMenu);
  }

}
