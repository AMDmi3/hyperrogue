#ifdef NOTKNOT
#include "../hyper.cpp"
#endif

#include "../hyper.h"

/** 

* mg 2 in old video

Blocky Knot Portal: 

compile with: mymake rogueviz/notknot

the video has been created with the following options:

older Euclidean

https://youtu.be/1TMY2U4_9Qg
nk_margin=2 -noplayer -canvas-random 20 -geo notknot -sight3 0.5 -ray-cells 600000 smooth_scrolling=1 camspd=10 panini_alpha=1 fov=150 -shot-hd ray_exp_decay_poly=30 ray_fixed_map=1 

better Euclidean

https://youtu.be/eb2DhCcGH7U
nk_margin=4 -noplayer -canvas-random 20 -geo notknot -sight3 0.5 -ray-cells 600000 smooth_scrolling=1 camspd=10 panini_alpha=1 fov=150 -shot-hd ray_exp_decay_poly=30 ray_fixed_map=1 -ray-iter 100 ray_reflect_val=0.30

self-hiding Euclidean

https://youtu.be/vFLZ2NGtuGw
selfhide=1 nk_loop=4 nk_margin=4 -noplayer -canvas-random 20 -geo notknot -sight3 0.5 -ray-cells 600000 smooth_scrolling=1 camspd=10 panini_alpha=1 fov=150 -shot-hd ray_exp_decay_poly=30 ray_fixed_map=1 -ray-iter 100 ray_reflect_val=0.30

penrose staircase in Nil

-noplayer nk_loop=6 nk_secondary=0 nk_terminate=9999999 -geo Nil -nilperiod 8 8 8 -nilwidth .25 -canvas-random 20 -nkbase -geo notknot ray_fixed_map=1 -ray-cells 600000 -sight3 1 -ray-range 1 3 -ray-random 1 -shot-1000

self-hiding knotted portal in S3

selfhide=1 -run nk_secondary=0 nk_terminate=9999999 nk_loop=4 -canvas-random 10 -load spherknot.lev -nkbase -nkbasemap spherknot.lev -PM nativ -fov 270 panini_alpha=1 -nk-volumetric -noplayer ray_fixed_map=1 -ray-cells 600000 -ray-iter 600

basic portal in S3:

nk_secondary=0 nk_terminate=99999 nk_loop=0 -nk-unloop 60 7 -canvas-random 10 -load spherring.lev -nkbase -nkbasemap spherring.lev -PM nativ -fov 270 panini_alpha=1 -nk-volumetric -noplayer



The algorithm here is as follows:

* create the map without portals (this is just the cube with the trifoil knot in it)

  This is done with a program (function create_trifoil_knot).
  The general algorithm should work with any map, and it would be possible to make it so that the user can create the map using HyperRogue map editor.

* the universal cover can be seen as the set of all paths, where two paths are identified if they are homotopic. So we generate all paths
  from the chosen starting point; when we generate extensions from path ...A, we check if we have a situation where we could go ...A->B->D or ...A->C->D --
  if so, the two paths are homotopic, so we identify them (that is, all the references to one of the paths are replaced with the references to the other path,
  possibly propagating to all known extensions of these paths)

* since the universal cover of the trifoil knot complement is infinite, we also identify two paths if they differ by three loops around the knot (around any point of the knot).
  (If we did not do that, the algorithm would never terminate, or we could terminate it at some point (terminate_at), but then we easily run into ugly unfinished areas.)

* this result in a relatively standard HyperRogue map representation (cubes connected via glued faces) which is rendered using HyperRogue's raycaster.

*/

namespace hr {

namespace notknot {

void create_notknot();

/* how many times we need to loop around the portal frame to get back to the same space */
/* the number of the worlds is: 1 (loop=1), 6 (loop=2), 24, 96, 600, infinity (loop>5) */
int loop = 3;

/* any loop repeated loop_any times */
int loop_any = 0;

/* extra space around the knot */
int margin = 4;

/* the scale factor for the knot */
int knotsize = 3;

int secondary_percentage = 10;

int terminate_at = 500000000;

string base_map = "";

/* make a self-hiding knot */
bool self_hiding = false;

eGeometry gNotKnot(eGeometry(-1));

/** It was easier to generate a program to design the trifoil knot than to generate it manually.
 *  This function does generate the instructions for trifoil knot, although they have been adjusted manually
 *  The generated instructions is the weird array in create_trifoil_knot()
 */

vector<vector<int> > to_unloop;

void gen_trifoil() {
  for(int len=2; len<=12; len+=2) {
    println(hlog, "trying len = ", len);
    int pos = 1;
    for(int l=0; l<len; l++) pos *= 6;
    
    for(int p=0; p<pos; p++) {

      vector<int> lst;
      int a = p;
      int bal = 0;
      for(int p=0; p<len; p++) {
        if(a%6 < 3) bal++; else bal--;
        lst.push_back(a%6);
        a /= 6;
        }
      if(bal) continue;
      
      array<int, 3> start = {0, 0, 0};
  
      array<int, 3> where = start;
  
      set<array<int, 3> > ful;
      map<array<int, 2>, int> proj;
      
      int steps = 0;
      vector<pair<int, int> > crosses;
      
      for(int i=0; i<3; i++) {
        for(auto d: lst) {
          if(ful.count(where)) goto next;
          ful.insert(where);
          auto pco = array<int,2>{{where[0]-where[2], where[1]-where[2]}};
          if(proj.count(pco)) crosses.emplace_back(proj[pco], steps);
          else proj[pco] = steps;
          where[(d+i)%3] += (d<3?1:-1);
          steps++;
          }
        }
      
      if(where != start) { println(hlog, "bad loop"); continue; }
      if(isize(ful) != 3*len) continue;

      if(isize(proj) != 3*len-3) continue;
      
      println(hlog, "len=", len, " ful=", isize(ful), " proj=", isize(proj), " for ", lst);
      
      println(hlog, "crosses = ", crosses);

      if(1) {
        set<int> crosval;
        for(auto c: crosses) crosval.insert(c.first), crosval.insert(c.second);
        vector<int> cvs;
        for(auto s: crosval) cvs.push_back(s);

        bool wrong = false;
        for(auto c: crosses) if(c.first == cvs[0] && c.second == cvs[1]) wrong = true;
        for(auto c: crosses) if(c.first == cvs[1] && c.second == cvs[2]) wrong = true;
      
        if(wrong) continue;
        }
      
      println(hlog, "result: ", lst);
      
      exit(3);

      next: ;
      }
    }
  
  exit(2);
  }

eGeometry base = gCubeTiling;

const int arrsize = 12;

struct hrmap_notknot : hrmap {
  
  /* represents a path (may be partially identified) */
  struct ucover {
    /* the heptagon of the underlying map */
    heptagon *where;
    /* the heptagon of the result map */
    heptagon *result;
    /* what is has been merged into */
    ucover *merged_into;
    /* connections in every direction */
    ucover *ptr[arrsize];
    /* used for painting walls in a single color */
    ucover *wall_merge;
    color_t wallcolor, wallcolor2;
    /* 0 = live, 1 = wall, 2 = merged, 4 = overflow, 8 = to hide */
    char state;
    /* direction to the parent */
    char parentdir;
    /* index in the table `all` */
    int index;
    ucover(heptagon *w, int s) { where = w; for(int i=0; i<arrsize; i++) ptr[i] = nullptr; state = s; merged_into = nullptr; result = nullptr; wall_merge = this; }
    bool iswall() { return state & 1; }
    bool nowall() { return !iswall(); }
    bool ismerged() { return state & 2; }
    bool isover() { return state & 4; }
    bool tohide() { return state & 8; }
    };
  
  /* find-union algorithm for wall_merge */
  ucover *ufind(ucover *at) {
    if(at->wall_merge == at) return at;
    return at->wall_merge = ufind(at->wall_merge);
    }

  void funion(ucover *a, ucover *b) {
    ufind(b)->wall_merge = ufind(a);
    }
  
  /* the vector of all paths */
  vector<ucover*> all;
  
  /* the stack of known unifications */
  vector<pair<ucover*, ucover*>> unify;
  
  /* the underlying map */
  hrmap *euc;

  heptagon *getOrigin() override {
    // return hepts[0];
    return all[0]->result;
    }  
  
  heptagon* at(int x, int y, int z) { 
    dynamicval<eGeometry> g(geometry, base);
    dynamicval<hrmap*> m(currentmap, euc);
    euc::coord co = euc::basic_canonicalize({x, y, z});
    
    return euc::get_at(co);
    }
  
  /* make sure that where->move(d) and where->c.spin(d) are known */
  void cmov(heptagon *where, int d) {
    if(where->move(d)) return;
    dynamicval<eGeometry> g(geometry, base);
    dynamicval<hrmap*> m(currentmap, euc);
    createStep(where, d);
    }
  
  heptagon *create_trifoil_knot() {
    euc::coord cmin{99,99,99}, cmax{-99,-99,-99}, cat{0,0,0};
    heptagon *h = at(0, 0, 0);
    
    vector<heptagon*> trifoil;
    
    int step = knotsize;

    for(int i=0; i<3; i++) {
      for(auto m: {3, 3, 3, 3, 5, 5, 1, 0, 0, 0, 0, 0}) for(int rep=0; rep<step; rep++) {
        trifoil.push_back(h);
        int d = (i+m)%3 + (m<3?0:3);
        cmov(h, d);
        h = h->move(d);
        if(m<3) cat[(i+m)%3]++; else cat[(i+m)%3]--;
        for(int k=0; k<3; k++) cmin[k] = min(cmin[k], cat[k]), cmax[k] = max(cmax[k], cat[k]);
        }
      }
    
    int& mg = margin;

    for(int i=cmin[0]-mg; i<=cmax[0]+mg; i++)
    for(int j=cmin[1]-mg; j<=cmax[1]+mg; j++)
    for(int k=cmin[2]-mg; k<=cmax[2]+mg; k++)
      if(among(i,cmin[0]-mg,cmax[0]+mg) || among(j,cmin[1]-mg,cmax[1]+mg) || among(k,cmin[2]-mg,cmax[2]+mg))
        at(i,j,k)->zebraval = 1;
      else
        at(i,j,k)->zebraval = 0;
      
    for(auto h: trifoil)
        h->zebraval = 9;
    
    return at(cmax[0], cmax[1], cmax[2]);
    }
  
  heptagon *create_nil_knot() {
    dynamicval<eGeometry> g(geometry, base);
    dynamicval<hrmap*> m(currentmap, euc);
    auto ac = currentmap->allcells();
    for(cell *c: ac) c->master->zebraval = 0;

    //ac[1]->master->zebraval = 1;
    
    auto hept = [&] (int x, int y, int z) {      
      x = zgmod(x, nilv::nilperiod[0]);
      y = zgmod(y, nilv::nilperiod[1]);
      z = zgmod(z, nilv::nilperiod[2]);
      return nilv::get_heptagon_at(nilv::mvec(x,y,z));
      };
    
    hept(-3, -3, -4)->zebraval |= 16;
      
    heptagon* h0 = hept(-2, -2, 0);
    auto h = h0;
    
    for(int d: {4, 3, 1, 0})
      for(int i=0; i<4; i++) {
        h->zebraval |= 1;
        h = h->cmove(d);
        h->zebraval |= 1;
        h = h->cmove(5);
        }

    if(h != h0) { println(hlog, "not looped"); exit(1); }
          
    hept(0, 0, 2)->zebraval |= 1;
    hept(1, 0, 2)->zebraval |= 1;
    // hept(2, 0, 2)->zebraval |= 1;
    hept(-1, 0, 2)->zebraval |= 1;
    // hept(-2, 0, 2)->zebraval |= 1;
    // hept(0, 1, 2)->zebraval |= 1;
    // hept(0, 2, 2)->zebraval |= 1;
    // hept(0, -1, 2)->zebraval |= 1;
    // hept(0, -2, 2)->zebraval |= 1;
    
    for(int z=0; z<8; z++)
    for(int x=-3; x<4; x++)
    for(int y=-3; y<4; y++)
      if(!(x>=-1 && x<=1 && y>=-1 && y<=1))
        if(!(hept(x,y,z)->zebraval & 1))
        hept(x, y, z)->zebraval |= 32;
    
    return ac[0]->master;
    }
  
  heptagon *interpret_basemap() {
    dynamicval<eGeometry> g(geometry, base);
    dynamicval<hrmap*> m(currentmap, euc);
    auto ac = currentmap->allcells();
    
    /*
    vector<cell*> wals;

    for(cell *c: ac) if(c->wall == waPlatform) {
      bool lone = true;
      forCellEx(d, c) if(d->wall == waPlatform) lone = false;
      if(!lone) wals.push_back(c);
      }
    cell *fst = nullptr;

    for(cell *c: ac) if(c->wall != waPlatform) {
      vector<int> dists;
      for(cell *w: wals) dists.push_back(celldistance(w, c));
      sort(dists.begin(), dists.end());
      int su = 0; for(auto s: dists) su += s;
      if(dists[0] >= 3 && !fst) fst = c;
      c->wall = dists[0] >= 3 && (c != fst) ? waFloorA : waNone;
      }

    vector<cell*> tochg;
    for(cell *c: ac) {
      if(c->wall == waPlatform) continue;
      if(c == fst || isNeighbor(c, fst)) continue;
      bool adj = false;
      forCellEx(d, c) if(d->wall == waFloorA) adj = true;
      if(adj) tochg.push_back(c);
      }
    
    for(cell *c: tochg) c->wall = waFloorA;
    */

/*
     50 000152   (1,1,2,2,3,3,4,4,4,4) SUM 28
     50 000152   (2,2,2,2,3,3,3,3,4,4) SUM 28
     10 000152   (3,3,3,3,3,3,3,3,3,3) SUM 30
*/    
    for(cell *c: ac) {
      auto& m = c->master->zebraval;
      m = 0;
      /* if(among(c->wall, waFloorA, waFloorB))
        m |= 32; */
      if(c->wall == waPlatform)
        m |= 9;
      }

    return ac[0]->master;
    }
  
  heptagon *create_under() {
    if(base_map != "")
      return interpret_basemap();
    if(base == gCubeTiling)
      return create_trifoil_knot();
    else if(base == gNil)
      return create_nil_knot();
    throw hr_exception();
    }
  
  bool remove_marked_walls;

  ucover *gen_adj(ucover *u, int d) {
    if(u->ptr[d]) return u->ptr[d];
    cmov(u->where, d);
    auto x = u->where->move(d);
    auto d1 = u->where->c.spin(d);
    auto z = x->zebraval;
    if(z & 6) {
      throw hr_exception_str("zebraval failure!");
      exit(3);
      x->zebraval = 0;
      }
    if(remove_marked_walls && (z & 8))
      z &=~ 9;
    u->ptr[d] = new ucover(x, z & 15);
    u->ptr[d]->ptr[d1] = u;
    u->ptr[d]->index = isize(all);
    u->ptr[d]->parentdir = d1;
    all.push_back(u->ptr[d]);
    return u->ptr[d];
    };
  
  void add_to_unify(ucover *a, ucover *b) {
    if(a->where != b->where)
      throw hr_exception_str("unification error");
    unify.emplace_back(a, b);
    };
  
  bool make_loop(ucover* u, int loopcount, const vector<int>& looplist) {
    auto ux = u;
    for(int iter=0; iter<loopcount; iter++)
      for(int w: looplist) {
        ux = gen_adj(ux, w);
        if(ux->iswall()) return false;
        }
    add_to_unify(u, ux);
    return true;
    };
    
  set<heptagon*> analyzed;
  
  transmatrix adj(ucover *u, int k) {
    dynamicval<eGeometry> g(geometry, base);
    dynamicval<hrmap*> m(currentmap, euc);
    return currentmap->adj(u->where, k);
    }

  bool adjacent_matrix(const transmatrix& Tk, const transmatrix& Tl) {
    for(auto vk: cgi.vertices_only)
    for(auto vl: cgi.vertices_only)
      if(hdist(Tk * vk, Tl * vl) < .01)
        return true;
    
    return false;
    }

  bool adjacent_face(ucover *u, int k, int l) {
    if(base == gCubeTiling) return abs(k-l) != 3;
    if(base == gNil) return abs(k-l) != 3;
    return adjacent_matrix(adj(u, k), adj(u, l));
    }

  void unify_homotopies(ucover *u) {
    /* unify homotopies */
    if(base == gNil) {
      make_loop(u, 1, {0, 2, 3, 5});
      make_loop(u, 1, {1, 2, 4, 5});
      make_loop(u, 1, {0, 1, 3, 4, 2});
      return;
      }
    if(base == gCubeTiling) {
      make_loop(u, 1, {0, 1, 3, 4});
      make_loop(u, 1, {0, 2, 3, 5});
      make_loop(u, 1, {1, 2, 4, 5});
      return;
      }
    
    for(auto& v: cgi.vertices_only) {
      map<heptagon*, ucover*> visited;
      vector<pair<ucover *, transmatrix>> q;
      
      auto visit = [&] (ucover *u, const transmatrix& T) {
        if(visited.count(u->where)) {
          add_to_unify(u, visited[u->where]);
          return;
          }
        visited[u->where] = u;
        q.emplace_back(u, T);
        };
      
      hyperpoint h = v;
      visit(u, Id);
      for(int i=0; i<isize(q); i++) {
        auto u1 = q[i].first;
        transmatrix T0 = q[i].second;
        for(int i=0; i<u1->where->type; i++) {
          auto u2 = gen_adj(u1, i);
          if(u2->state != 0) continue;
          auto T1 = T0 * adj(u1, i);
          bool adjacent = false;
          for(auto& v2: cgi.vertices_only)
            if(hdist(T1 * v2, h) < 1e-5)
              adjacent = true;
          if(adjacent)
            visit(u2, T1);
          }
        }
      }
    }
  
  map<heptagon*, vector<vector<int> > > uloops;

  void unify_loops_general(ucover *u) {
    int t = u->where->type;
    if(uloops.count(u->where)) {
      for(auto& ul: uloops[u->where])
        make_loop(u, loop, ul);
      return;
      }
    uloops[u->where] = {};
    for(int i=0; i<t; i++) {
      auto u1 = gen_adj(u, i);
      if(u1->nowall()) continue;
      if(u1->where->zebraval != 9) continue;
      transmatrix M = adj(u, i);
      
      map<heptagon*, int> camefrom;
      vector<pair<ucover*, transmatrix>> visited;
      
      auto visit = [&] (ucover *from, ucover *at, int ldir, const transmatrix& T) {
        // println(hlog, from ? from->where : (heptagon*)nullptr, " -> ", at->where, " (", i, ")", " (reverse ", at->where->c.spin(i), ")");
        if(camefrom.count(at->where)) {
          vector<int> path;
          vector<int> rpath;

          while(at->where != u->where) {
            int d = camefrom[at->where];
            // println(hlog, "from ", at->where, " going back ", d);
            rpath.push_back(at->where->c.spin(d));
            at = gen_adj(at, d);
            }
          while(!rpath.empty()) { path.push_back(rpath.back()); rpath.pop_back(); }
          path.push_back(ldir);
          
          int st = 0;

          while(from->where != u->where) {
            int d = camefrom[from->where];
            // println(hlog, "from ", from->where, " going ", d);
            st++; if(st == 10) exit(1);
            path.push_back(d);
            from = gen_adj(from, d);
            }
          
          if(isize(path) == 8 && path[1] == path[2] && path[3] == path[4] && path[5] == path[6]) {
            println(hlog, "path = ", path, " i=", i);
            }
          
          uloops[u->where].push_back(path);
          make_loop(u, loop, path);
          }
        else {
          camefrom[at->where] = ldir;
          visited.emplace_back(at, T);
          }
        };
      
      visit(nullptr, u, -1, Id);
      for(int vi=0; vi<isize(visited); vi++) {
        auto at = visited[vi].first;
        auto Ti = visited[vi].second;

        for(int j=0; j<at->where->type; j++) {
          if(j == camefrom[at->where]) continue;
          auto u2 = gen_adj(at, j);          
          if(u2->iswall()) continue;
          transmatrix Tj = Ti * adj(at, j);
          bool adj = false;
          for(auto v: cgi.vertices_only) for(auto v1: cgi.vertices_only)
            if(hdist(M * v1, Tj * v) < 1e-3) adj = true;
          
          if(adj) visit(at, u2, at->where->c.spin(j), Tj);
          }
        }
      }
    }
  
  void unify_loops(ucover *u) {
    /* try to make it finite */
    bool special = false;
    
    if(base_map == "" && base == gNil) {
      special = true;
      make_loop(u, loop, {0, 0, 2, 2, 2, 3, 3, 5, 5, 5});
      if(u->where->zebraval & 16)
      for(int dir: {0,1,2}) {
        auto ux = u;
        for(int iter=0; iter<nilv::nilperiod[dir]; iter++) {
          ux = gen_adj(ux, dir);
          if(ux->state != 0) goto next_dir;
          }
        println(hlog, "succeeded in direction ", dir);
        add_to_unify(u, ux);
        next_dir: ;
        }
      }
    
    if(base == gCubeTiling) {
      special = true;
      make_loop(u, loop, {0, 0, 1, 1, 3, 3, 4, 4});
      }
    
    if(base == gCell120) {
      special = true;
      int t = u->where->type;
      for(int i=0; i<t; i++) 
      for(int j=0; j<i; j++) {
        auto u1 = u->ptr[i];
        auto u2 = u->ptr[j];
        if(u1->nowall()) continue;
        if(u2->nowall()) continue;
        auto ucur = u;
        auto ulast = (ucover*) nullptr;
        
        for(int step=0; step<5*loop; step++) {
          for(int i=0; i<t; i++) {
            auto ucand = ucur->ptr[i];
            if(ucand && isNeighbor(ucand->where->c7, u1->where->c7) && isNeighbor(ucand->where->c7, u2->where->c7) && ucand != ulast) {
              ulast = ucur, ucur = ucand; 
              goto next_step;
              }
            }
          goto fail;
          next_step: ;
          }
        
        add_to_unify(u, ucur);
        fail: ;
        }
      }

    if(loop && !special)
      unify_loops_general(u);
    
    if(u->where == all[0]->where)
      for(auto& lo: to_unloop)        
        if(!make_loop(u, 1, lo))
          throw hr_exception_str("given loop goes through a wall");
    
    if(loop_any && u->where == all[0]->where) {
      auto us = all[0];
      for(int it=1; it<loop_any; it++) {
        vector<int> pathback;
        auto uc = u;
        while(uc->parentdir != -1) {
          pathback.push_back(uc->parentdir);
          us = gen_adj(us, uc->parentdir);
          uc = uc->ptr[(int) uc->parentdir];
          }
        if(it == 1) println(hlog, "pathback = ", pathback);
        }
      add_to_unify(us, u);
      }
    }
  
  map<heptagon*, int> indices;
  
  hrmap_notknot() {

    try {
    if(base_map != "") {
      dynamicval<eGeometry> dg(geometry, geometry);
      mapstream::loadMap(base_map);
      base = geometry;    
      create_notknot();
      euc = currentmap;
      for(hrmap*& m: allmaps) if(m == euc) m = NULL;
      }
    else {
      dynamicval<eGeometry> dg(geometry, base);
      initcells(); euc = currentmap;
      for(hrmap*& m: allmaps) if(m == euc) m = NULL;
      }
    
    int i = 0;

    all.emplace_back(new ucover(create_under(), 0));
    all[0]->index = 0;
    all[0]->parentdir = -1;

    if(all[0]->where->zebraval & 1) 
      throw hr_exception_str("error: starting inside a wall");
    
    remove_marked_walls = false;
    bool first = true;

    back:
    
    while(true) {
    
      /* handle all known unifications */
      if(!unify.empty()) {
        ucover *uf, *ut;
        tie(uf, ut) = unify.back();
        unify.pop_back();
        while(uf->merged_into) uf = uf->merged_into;
        while(ut->merged_into) ut = ut->merged_into;
        if(uf == ut) continue;
        if(!uf || !ut) println(hlog, "null unified");
        /* we always keep the one with the lower index */
        if(uf->index < ut->index) swap(uf, ut);

        /* if a knot is removed, remove the other copy */
        if(uf->iswall() && ut->nowall())
          uf->state &=~ 1;

        uf->state |= 2; uf->merged_into = ut;
        if(uf->where != ut->where)
          throw hr_exception_str("where confusion");
        for(int d=0; d<uf->where->type; d++) {
          cmov(uf->where, d);
          auto d1 = uf->where->c.spin(d);
          if(uf->ptr[d]) {
            if(!ut->ptr[d]) {
              /* was a known connection in uf, but not in ut -- reconnect it to ut */
              uf->ptr[d]->ptr[d1] = ut;
              ut->ptr[d] = uf->ptr[d];
              uf->ptr[d] = nullptr;
              }
            else {
              /* in some direction, connections for both uf and ut are already known, so unify them too */
              add_to_unify(uf->ptr[d], ut->ptr[d]);
              uf->ptr[d]->ptr[d1] = nullptr;
              uf->ptr[d] = nullptr;
              }
            }
          }
        continue;
        }
    
      /* handle creation and loops */

      if(i >= isize(all)) break;
      auto u = all[i++];
      if(u->state != 0) continue;
      if(i > terminate_at) { u->state |= 4; continue; }
      
      for(int k=0; k<u->where->type; k++) gen_adj(u, k);
      
      if(!analyzed.count(u->where)) {
        analyzed.insert(u->where);
        unify_homotopies(u);
        unify_loops(u);
        }
      unify_homotopies(u);
      unify_loops(u);
      }
    
    /* make the walls single-colored */
    
    println(hlog, "single-colored");

    for(int i=0; i<isize(all); i++) {
      auto u = all[i];
      if(u->state != 0) continue;
      
      if(u->where->zebraval & 32) {
        for(int k=0; k<u->where->type; k++) {
          auto uk = gen_adj(u, k);
          if(uk->state != 0) continue;
          if(uk->where->zebraval & 32)
            funion(u, uk);
          }
        }

      /* convex corners */
      for(int k=0; k<u->where->type; k++)
      for(int l=0; l<u->where->type; l++) {
        auto uk = gen_adj(u, k);
        if(uk->state != 0) continue;
        auto ul = gen_adj(u, l);
        if(ul->state != 0) continue;
        if(geometry == gCubeTiling || geometry == gNil) {
          auto ukl = gen_adj(uk, l);
          auto ulk = gen_adj(ul, k);
          if(ukl->where == ulk->where && ukl->state != 0)
            funion(ukl, ulk);
          }
        else {
          for(int k1=0; k1<u->where->type; k1++)
          for(int l1=0; l1<u->where->type; l1++) {
            auto ukl = gen_adj(uk, l1);
            auto ulk = gen_adj(ul, k1);
            if(ukl->where == ulk->where && ukl->state != 0)
              funion(ukl, ulk);
            if(base == gCell600 && isNeighbor(ukl->where->c7, ulk->where->c7) && ukl->state != 0 && ulk->state != 0)
              funion(ukl, ulk);

            if(base == gCell600 && ulk->nowall() && ukl->iswall()) {
              for(int m1=0; m1<u->where->type; m1++) {
                auto ulkm = gen_adj(ulk, m1);
                if(ulkm->where == ukl->where) funion(ulkm, ukl);
                }
              }
            }
          }
        }

      /* flat areas */
      for(int k=0; k<u->where->type; k++)
      for(int l=0; l<u->where->type; l++) {
        auto uk = gen_adj(u, k);
        if(uk->state != 0) continue;
        auto ul = gen_adj(u, l);
        if(ul->nowall()) continue;
        auto ukl = gen_adj(uk, l);
        if(ukl->nowall()) continue;
        funion(ul, ukl);        
        }

      /* concave corners */
      for(int k=0; k<u->where->type; k++)
      for(int l=0; l<u->where->type; l++) 
      if(adjacent_face(u, k, l)) {
        auto uk = gen_adj(u, k);
        if(uk->nowall()) continue;
        auto ul = gen_adj(u, l);
        if(ul->nowall()) continue;
        funion(ul, uk);        
        }      

      if(base == gCell600) 
      for(int k=0; k<u->where->type; k++)
      for(int l=0; l<u->where->type; l++) 
      if(adjacent_face(u, k, l)) {
        auto uk = gen_adj(u, k);
        if(uk->nowall()) continue;
        auto ul = gen_adj(u, l);
        if(ul->iswall()) continue;

        for(int m=0; m<u->where->type; m++)
          if(adjacent_matrix(adj(u, k), adj(u, l) * adj(ul, m))) {
          auto um = gen_adj(ul, m);
          if(um->nowall()) continue;
          funion(um, uk);
          }
        }
      }
    
    /* statistics */
    int lives = 0, walls = 0, overflow = 0, merged = 0;
    
    for(auto v: all) {
      if(v->state == 0) lives++;
      if(v->iswall()) walls++;
      if(v->ismerged()) merged++;
      if(v->isover()) overflow++;
      }
      
    set<heptagon*> wheres;
    
    println(hlog, "lives = ", lives);
    println(hlog, "walls = ", walls);
    println(hlog, "merged = ", merged);
    println(hlog, "overflow = ", overflow);
    println(hlog, "total = ", isize(all));

    /* create the result map */
    for(int i=0; i<isize(all); i++) {
      auto u = all[i];
      if(u->ismerged()) continue;
      if(u->state == 0) wheres.insert(all[i]->where);
      u->result = tailored_alloc<heptagon> (S7);      
      u->result->c7 = newCell(S7, u->result);
      indices[u->result] = i;
      }

    println(hlog, "wheres = ", isize(wheres), " : ", lives * 1. / isize(wheres));

    for(int i=0; i<isize(all); i++) {
      auto u = all[i];
      if(u->ismerged()) continue;

      for(int d=0; d<S7; d++) {        
        cmov(u->where, d);
        auto d1 = u->where->c.spin(d);
        if(u->ptr[d] && u->ptr[d]->result == nullptr)
          throw hr_exception_str(lalign(0, "connection to null in state ", u->ptr[d]->state, " from state ", u->state, " i=", i, " .. ", u->ptr[d]->index));
        if(u->ptr[d] && u->ptr[d]->ptr[d1] != u)
          throw hr_exception_str("wrong connection");
        if(u->ptr[d])
          u->result->c.connect(d, u->ptr[d]->result, d1, false);          
        else
          u->result->c.connect(d, u->result, gmod(d + u->result->type/2, u->result->type), false);
        }
      }
    
    for(int k=0; k<23; k++) hrand(5);

    int colors_used = 0;

    for(int i=0; i<isize(all); i++) 
      all[i]->wallcolor = 0;

    for(int i=0; i<isize(all); i++) 
      if(all[i]->iswall() && !all[i]->ismerged())
        ufind(all[i])->wallcolor++;
        
    map<int, int> sizes;

    for(int i=0; i<isize(all); i++) 
      if(all[i]->iswall() && ufind(all[i]) == all[i] && all[i]->wallcolor)
        colors_used++,
        sizes[all[i]->wallcolor]++;

    for(auto p: sizes) 
      println(hlog, "size = ", p.first, " times ", p.second);
    
    println(hlog, "colors_used = ", colors_used);
    
    if(first && self_hiding) {
      ucover *what = nullptr;
      for(int i=0; i<isize(all); i++) 
        if(all[i]->iswall() && all[i]->tohide() && !all[i]->ismerged())
          what = ufind(all[i]);

      for(int i=0; i<isize(all); i++) 
        if(all[i]->iswall() && ufind(all[i]) == what) 
          all[i]->state &=~ 9;
        
      println(hlog, "removed one knot!");
        
      first = false; i = 0; remove_marked_walls = true;
      goto back;
      }

    for(int i=0; i<isize(all); i++) 
      if(all[i]->iswall() && ufind(all[i]) == all[i]) {
        all[i]->wallcolor = hrand(0x1000000) | 0x404040,
        all[i]->wallcolor2 = hrand(0x1000000) | 0x404040;
        }

    for(int i=0; i<isize(all); i++) 
      if((all[i]->where->zebraval & 32) && ufind(all[i]) == all[i]) {
        auto& w = all[i]->wallcolor;
        all[i]->wallcolor = (hrand(0x1000000) << 8) | 0x01;
        switch(hrand(6)) {
          case 0: w |= 0xFF000000; break;
          case 1: w |= 0x00FF0000; break;
          case 2: w |= 0x0000FF00; break;
          case 3: w |= 0xC0C00000; break;
          case 4: w |= 0x00C0C000; break;
          case 5: w |= 0xC000C000; break;
          }
        }

    for(int i=0; i<isize(all); i++) {
      auto u = all[i];
      if(!u->result) continue;
      cell *c = u->result->c7;
      setdist(c, 7, c);
      c->land = laCanvas;
      if(u->iswall()) {
        c->wall = waWaxWall;
        c->landparam = hrand(100) < secondary_percentage ? ufind(u)->wallcolor2 : ufind(u)->wallcolor;
        if(ufind(u)->nowall()) println(hlog, "connected to state ", ufind(u)->state);
        // if(!(c->landparam & 0x404040)) println(hlog, "color found ", c->landparam);
        }
      else if(u->isover())
        c->wall = waBigTree;
      else 
        c->wall = waNone;

      if(all[i]->where->zebraval & 32)       
        ray::volumetric::vmap[c] = ufind(u)->wallcolor ^ ((hrand(0x1000000) & 0x3F3F3F) << 8);
      else
        ray::volumetric::vmap[c] = 0x00000001;
      }
    
    } catch(hr_exception_str& s) {
      println(hlog, "exception: ", s.s);
      throw;
      }
    }

  void add_fog() {
    vector<color_t> cols = {0xFF000001, 0xC0C00001, 0x00FF0001, 0x00C0C001, 0x0000FF01, 0xC000C001};
    int id = 0;
    map<cell*, pair<int, int> > dist;
    vector<cell*> lst;
    
    auto color = [&] (cell *c, color_t col, int d) {
      if(!dist.count(c)) dist[c] = {d, 0};
      auto& p = dist[c];
      if(p.first == d) {
        if(!p.second) lst.push_back(c);
        p.second++;
        auto& vm = ray::volumetric::vmap[c];
        if(p.second == 1) vm = col;
        else vm = gradient(vm, col, 0, 1, p.second);
        }
      };
    
    for(int i=0; i<isize(all); i++) 
      if(all[i]->result)
      if(all[i]->where->c7->wall == waFloorA)
        color(all[i]->result->c7, cols[(id++) % isize(cols)], 0);
    
    for(int i=0; i<isize(lst); i++) {
      auto c = lst[i];
      auto col = ray::volumetric::vmap[c];
      int d = dist[c].first;
      forCellCM(c1, c)
        if(c1->wall == waNone)
          color(c1, col, d+1);
      }

    for(int i=0; i<isize(lst); i++) {
      auto c = lst[i];
      ray::volumetric::vmap[c] ^= ((hrand(0x1000000) & 0x3F3F3F) << 8);
      }
    
    ray::volumetric::enable();
    }
  
  transmatrix relative_matrix(heptagon *h2, heptagon *h1, const hyperpoint& hint) override {
    return Id;
    }

  transmatrix adj(heptagon *h, int i) override {
    return adj(all[indices[h]], i);
    }

  transmatrix adj(cell *c, int i) override {
    return adj(c->master, i);
    }
    
  ~hrmap_notknot() {
    for(auto uc: all) {
      if(uc && uc->result) {
        tailored_delete(uc->result->c7);
        tailored_delete(uc->result);
        }
      if(uc) delete uc;        
      }
    delete euc;
    }
  
  };

auto h = addHook(hooks_newmap, 0, [] {
  // gen_trifoil();
  if(geometry == gNotKnot) {
    return (hrmap*) new hrmap_notknot;
    }
  return (hrmap*) nullptr;
  });

void create_notknot() {
  if(gNotKnot == eGeometry(-1)) {
    ginf.push_back(ginf[base]);
    gNotKnot = eGeometry(isize(ginf) - 1);
    }
  else ginf[gNotKnot] = ginf[base];
  // variation = eVariation::pure;
  auto& gi = ginf.back();
  gi.flags |= qANYQ | qBOUNDED | qEXPERIMENTAL | qPORTALSPACE;
  gi.quotient_name = "notknot";
  gi.shortname = "notknot";
  gi.menu_displayed_name = "notknot";
  }

void regenerate() {
  if(geometry == gNotKnot && game_active) {
    stop_game();
    start_game();
    }
  }

void show() {
  cmode = sm::SIDE | sm::MAYDARK;
  gamescreen(0);
  dialog::init(XLAT("notknot"), 0xFFFFFFFF, 150, 0);
  
  add_edit(loop);
  add_edit(margin);
  add_edit(knotsize);
  add_edit(self_hiding);
  
  dialog::addBreak(100);

  dialog::addItem(XLAT("configure raycasting"), 'A');
  dialog::add_action_push(ray::configure);

  #if CAP_VR
  dialog::addBoolItem(XLAT("VR settings"), vrhr::active(), 'v');
  dialog::add_action_push(vrhr::show_vr_settings);
  #endif

  dialog::addBack();
  dialog::display();    
  }

void o_key(o_funcs& v) {
  if(geometry == gNotKnot) v.push_back(named_dialog("notknot", show));
  }

bool do_check_cycle;
cell *startcell, *current;
vector<int> dirs;

void check_cycle() {
  if(!do_check_cycle) return;
  if(!current) {
    auto s = currentmap->allcells()[0];
    println(hlog, "starting the cycle, ", cwt.at == s);
    startcell = current = cwt.at = s;
    }
  if(cwt.at != current) {
    forCellIdEx(c1, i, current)
      if(c1 == cwt.at) {
        dirs.push_back(i);
        current = cwt.at;
        startcell->item = itGold;
        println(hlog, "dirs = ", dirs, " finished = ", startcell == current);
        string dirstr;
        for(int d: dirs)
          if(d < 10)
            dirstr += char('0' + d);
          else
            dirstr += char('a' + d-10);
        addMessage("this loop can be identified with identity using: -nk-unloop 1 " + dirstr);
        }
    }
  }

void gen_knot() {
  for(cell *c: currentmap->allcells())
    c->wall = waNone;
  
  cell *last = nullptr;
  
  for(int i=0; i<3600; i++) {
    ld alpha = i * degree / 10;
    ld q = sqrt(2)/2;
    hyperpoint h = hyperpoint(q*cos(alpha*2), q*sin(alpha*2), q*cos(alpha*3), q*sin(alpha*3));
    cell *b = currentmap->gamestart();
    virtualRebase(b, h);
    b->wall = waPlatform;
    if(b != last) {
      if(!last) println(hlog, "start at ", b);
      if(last) println(hlog, "i=", i, ": to ", b, " isN = ", isNeighbor(last, b));
      last = b;
      }
    }
  }

auto shot_hooks = addHook(hooks_initialize, 100, create_notknot)
  + addHook(hooks_welcome_message, 100, [] {
    if(geometry == gNotKnot) {
      addMessage("Welcome to Notknot! Press 'o' for options");
      return true;
      }
    return false; 
    })
  + addHook(hooks_args, 100, [] {
    using namespace arg;
             
    if(0) ;
    else if(argis("-nkbase")) {
      base = geometry;
      create_notknot();
      }
    else if(argis("-nkbasemap")) {
      shift(); base_map = args();
      set_geometry(gNotKnot);
      }
    else if(argis("-nk-volumetric")) {
      start_game();
      ((hrmap_notknot*)currentmap)->add_fog();
      }
    else if(argis("-nk-genknot")) {
      start_game();
      gen_knot();
      }
    else if(argis("-nk-findloop")) {
      do_check_cycle = true;
      }
    else if(argis("-nk-unloop")) {
      shift();
      int copies = argi();
      shift();
      vector<int> v;
      for(int i=0; i<copies; i++)
      for(char c: args()) 
        if(c >= '0' && c <= '9') v.push_back(c - '0');
        else v.push_back(c - 'a' + 10);
      to_unloop.push_back(v);
      println(hlog, "pushed to to_unloop: ", v);
      }
    else return 1;
    return 0;
    })

  + addHook(hooks_o_key, 80, o_key)
  + addHook(hooks_frame, 100, check_cycle)
  + addHook(hooks_configfile, 100, [] {
    param_i(loop, "nk_loop")
    ->editable(1, 5, 1, "notknot order", "How many times do we need to go around the knot to get back.", 'o')
    ->set_sets([] { dialog::bound_low(1); dialog::bound_up(5); })
    ->set_reaction(regenerate);
    param_i(margin, "nk_margin")
    ->editable(0, 10, 1, "notknot margins", "Empty space close to the walls.", 'm')
    ->set_sets([] { dialog::bound_low(0); dialog::bound_up(10); })
    ->set_reaction(regenerate);
    param_i(knotsize, "nk_knotsize")
    ->editable(0, 10, 1, "notknot size", "Size of the knot.", 's')
    ->set_sets([] { dialog::bound_low(2); dialog::bound_up(5); })
    ->set_reaction(regenerate);
    param_i(terminate_at, "nk_terminate")->set_reaction(regenerate);
    param_i(secondary_percentage, "nk_secondary");
    param_b(self_hiding, "selfhide")
    ->editable("self-hiding knot", 'h')
    ->set_reaction(regenerate);
    param_i(loop_any, "nk_loopany");
    });

#ifdef NOTKNOT
auto hook1=
    addHook(hooks_config, 100, [] {
      if(arg::curphase == 1) 
        conffile = "notknot.ini";         
      if(arg::curphase == 2) {
        margin = 4;
        mapeditor::drawplayer = false;
        stop_game();
        firstland = specialland = laCanvas;
        set_geometry(gNotKnot);
        sightranges[geometry] = .5;
        ray::max_cells = 600000;
        smooth_scrolling = 1;
        camera_speed = 10;
        // panini_alpha = 1;
        // fov = 150;
        ray::exp_decay_poly = 30;
        ray::fixed_map = true;
        ray::max_iter_iso = 80;
        showstartmenu = false;
        #if CAP_VR
        vrhr::hsm = vrhr::eHeadset::holonomy;
        vrhr::eyes = vrhr::eEyes::truesim;
        vrhr::cscr = vrhr::eCompScreen::eyes;
        vrhr::absolute_unit_in_meters = 0.2;
        #endif
        // ray::reflect_val = 0.3;
        }
      });
#endif

}

}
