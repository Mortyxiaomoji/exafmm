#include <cstdlib>
#include <iostream>
#include <vector>

typedef double real_t;                                        //!< Floating point type is double precision

struct Body {
  real_t X[2];                                                //!< Position
  real_t q;
};
typedef std::vector<Body> Bodies;                             //!< Vector of bodies

struct Cell {
  int NCHILD;                                                 //!< Number of child cells
  int NBODY;                                                  //!< Number of descendant bodies
  Cell * CHILD;                                               //!< Pointer to first child cell
  Body * BODY;                                                //!< Pointer to first body
  real_t X[2];                                                //!< Cell center
  real_t R;                                                   //!< Cell radius
  real_t M;
};
typedef std::vector<Cell> Cells;                              //!< Vector of cells

void buildTree(Bodies & bodies, Cells & cells, Cell * cell, real_t * Xmin, real_t * X0, real_t R0, int begin, int end, int ncrit) {
  cell->BODY = &bodies[0] + begin;
  cell->NBODY = end - begin;
  cell->NCHILD = 0;
  cell->M = 0;
  for (int d=0; d<2; d++) cell->X[d] = X0[d];
  cell->R = R0;
  if (end - begin < ncrit) return;
  // Count bodies in each quadrant
  int size[4] = {0};
  for (size_t b=begin; b<end; b++) {
    int quadrant = ((bodies[b].X[0] - Xmin[0]) > X0[0]) + (((bodies[b].X[1] - Xmin[1]) > X0[1]) << 1);
    size[quadrant]++;
  }
  cell->NCHILD = 0;
  for (int i=0; i<4; i++)
    if (size[i]) cell->NCHILD++;
  // Calculate offset of each quadrant
  int counter[4] = {begin, begin, begin, begin};
  for (int i=1; i<4; i++) {
    counter[i] = size[i-1] + counter[i-1];
  }
  // Sort bodies
  Bodies buffer = bodies;
  for (size_t b=begin; b<end; b++) {
    int quadrant = ((buffer[b].X[0] - Xmin[0]) > X0[0]) + (((buffer[b].X[1] - Xmin[1]) > X0[1]) << 1);
    bodies[counter[quadrant]] = buffer[b];
    counter[quadrant]++;
  }
  cells.resize(cells.size()+cell->NCHILD);
  Cell * child = &cells.back() - cell->NCHILD + 1;
  cell->CHILD = child;
  // Calculate new center and radius
  real_t X[2], R;
  int c = 0;
  for (int i=0; i<4; i++) {
    R = R0 / 2;
    for (int d=0; d<2; d++) {
      X[d] = X0[d] + R * (((i & 1 << d) >> d) * 2 - 1);
    }
    // Recursive call only if size[i] != 0
    if (size[i]) {
      buildTree(bodies, cells, &child[c], Xmin, X, R, counter[i]-size[i], counter[i], ncrit);
      c++;
    }
  }
}

void P2M(Cell * cell) {
  for(int i=0; i<cell->NBODY; i++) {
    Body * body = cell->BODY+i;
    cell->M += body->q;
  }
}

void M2M(Cell * cell) {
  for(int i=0; i<cell->NCHILD; i++) {
    Cell * child = cell->CHILD+i;
    cell->M += child->M;
  }
}

void upwardPass(Cell * cell) {
  for(int i=0; i<cell->NCHILD; i++) {
    upwardPass(cell->CHILD+i);
  }
  if(cell->NCHILD==0) P2M(cell);
  else M2M(cell);
}

int main(int argc, char ** argv) {
  int ncrit = 4;                                                // Number of bodies per leaf cell
  const int numBodies = 60;                                     // Number of bodies
  // Initialize bodies
  Bodies bodies(numBodies);
  for (size_t b=0; b<numBodies; b++) {                          // Loop over bodies
    for (int d=0; d<2; d++) {                                   //  Loop over dimension
      bodies[b].X[d] = drand48();                               //   Initialize coordinates
    }                                                           //  End loop over dimension
    bodies[b].q = 1;
  }                                                             // End loop over bodies

  // Get bounds
  real_t Xmin[2], Xmax[2];
  Xmin[0] = Xmax[0] = bodies[0].X[0];
  Xmin[1] = Xmax[1] = bodies[0].X[1];
  for (size_t b=0; b<numBodies; b++) {
    for (size_t d=0; d<2; d++){
      Xmin[d] = Xmin[d] > bodies[b].X[d] ? bodies[b].X[d] : Xmin[d];
      Xmax[d] = Xmax[d] < bodies[b].X[d] ? bodies[b].X[d] : Xmax[d];
    }
  }
  // Get center and radius
  real_t X0[2], R0;
  X0[0] = (Xmin[0] + Xmax[0]) / 2;
  X0[1] = (Xmin[1] + Xmax[1]) / 2;
  R0 = std::max(Xmax[0] - Xmin[0], Xmax[1] - Xmin[1]) * .50001;
  Xmin[0] = X0[0] - R0;
  Xmax[0] = X0[0] + R0;
  Xmin[1] = X0[1] - R0;
  Xmax[1] = X0[1] + R0;

  Cells cells(1);
  cells.reserve(numBodies);
  cells[0].X[0] = X0[0];
  cells[0].X[1] = X0[1];
  cells[0].R = R0;
  buildTree(bodies, cells, &cells[0], Xmin, X0, R0, 0, numBodies, ncrit);

  upwardPass(&cells[0]);
  std::cout << cells[0].M << " " << numBodies << std::endl;
  return 0;
}