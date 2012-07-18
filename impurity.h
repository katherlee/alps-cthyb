/*****************************************************************************
 *
 * ALPS DMFT Project
 *
 * Copyright (C) 2005 - 2009 by Philipp Werner <werner@itp.phys.ethz.ch>,
 *                              Emanuel Gull <gull@phys.columbia.edu>,
 *
 *
* This software is part of the ALPS Applications, published under the ALPS
* Application License; you can use, redistribute it and/or modify it under
* the terms of the license, either version 1 or (at your option) any later
* version.
* 
* You should have received a copy of the ALPS Application License along with
* the ALPS Applications; see the file LICENSE.txt. If not, the license is also
* available from http://alps.comp-phys.org/.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
* DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/

#ifndef ___IMP___
#define ___IMP___

#include <alps/scheduler/montecarlo.h>

#include <stack>
#include <queue>
#include <vector>
#include <iostream>
#include <algorithm>
#include <alps/alea.h>
#include <alps/osiris/dump.h>
#include <alps/osiris/std/vector.h>
#include <cmath>
#include <alps/osiris/std/list.h>
#include <alps/numeric/matrix.hpp>
#include "green_function.h"
#include "U_matrix.h"
#include "alps_solver.h"

typedef alps::numeric::matrix<double> alps_matrix;
class times
{
public:
  times() {t_start_=0; t_end_=0; }
  times(double t_start, double t_end) {t_start_=t_start; t_end_=t_end; }
  times(const times &t){
    t_start_=t.t_start_;
    t_end_=t.t_end_;
  }
  const double &t_start() const {return t_start_;} // begin of segment
  const double &t_end() const {return t_end_;} // end of segment
  void set_t_start(double t_start) {t_start_ = t_start; }
  void set_t_end(double t_end) {t_end_ = t_end;}
  
private:
  double t_start_, t_end_;
};

inline bool operator<(const times& t1, const times& t2) {
  return t1.t_start() < t2.t_start();
}
inline bool operator<(const times& t1, const double t2) {
  return t1.t_start() < t2;
}


inline bool operator<(const double t1, const times& t2) {
  return t1 < t2.t_start();
}

inline bool operator>(times t1, times t2) {
  return t1.t_start() > t2.t_start();
}

inline bool operator==(times t1, times t2) {
  return t1.t_start() == t2.t_start();
}

inline alps::ODump& operator << (alps::ODump& odump, const times& t) {
  odump << t.t_start() << t.t_end();
  return odump;
}

inline alps::IDump& operator >> (alps::IDump& idump, times& t) {
  double dummy;
  idump >> dummy;
  t.set_t_start(dummy);
  idump >> dummy;
  t.set_t_end(dummy);
  return idump;
}

//typedef std::list<times> segment_container_t;
typedef std::vector<times> segment_container_t;
//typedef std::set<times> segment_container_t;

class HybridizationRun : public alps::scheduler::MCRun
{
public:
  HybridizationRun(const alps::ProcessList&,const alps::Parameters&,int);
  HybridizationRun(int,const alps::ProcessList&,alps::IDump&,int);
  //void save(alps::ODump&) const;
  //void load(alps::IDump&);
  void dostep();
  void measure_fourpoint();
  bool is_thermalized() const;
  double work_done() const;
  bool change_parameter(const std::string& name, const alps::StringValue& value);
  
private:
  int sweeps;                        // sweeps done
  int thermalization_sweeps;        // sweeps to be done for equilibration
  int total_sweeps;                    // sweeps to be done after equilibration
  int start_time;            //time when solver started
  int max_time;                //maximum time for solver
  double mu;                        // chemical potential
  double BETA;                    // BETA
  //double u00;                        // on-site interaction
  U_matrix u;                    
  double t;                            // bandwidth=4t (for semi-circle)
  std::vector<std::vector<double> > F;            // F_up(\tau) = -G_{0,down}^{-1}(-\tau) + (iw + mu) hybridization function
  std::vector<segment_container_t >    segments;        // stores configurations with 0,1,... segments (but not full line)
  std::vector<int>                    full_line;                    // if 1 means that particle occupies full time-line
  std::vector<double>                sign;                    // sign of Z_n_up
  std::vector<alps_matrix>            M;                // inverse matrix for up-spins
  std::valarray<double> G_meas;
};

class HybridizationSimFrequency : public alps::scheduler::MCSimulation, public alps::MatsubaraImpurityTask
{
public:
  void write_fourpoint() const;
  HybridizationSimFrequency(const alps::ProcessList& w, const boost::filesystem::path& p) 
  : alps::scheduler::MCSimulation(w,p)
  { 
  }
  
  HybridizationSimFrequency(const alps::ProcessList& w, const alps::Parameters& p) 
  : alps::scheduler::MCSimulation(w,p) 
  {
    p_=p;
  }
  
    ///return the Green's function G
  std::pair<matsubara_green_function_t, itime_green_function_t> get_result();
private:
  alps::Parameters p_;
  
};

class HybridizationSimItime: public alps::scheduler::MCSimulation, public alps::ImpurityTask
{
public:
  HybridizationSimItime(const alps::ProcessList& w, const boost::filesystem::path& p) 
  : alps::scheduler::MCSimulation(w,p)
  { 
  }
  
  HybridizationSimItime(const alps::ProcessList& w, const alps::Parameters& p) 
  : alps::scheduler::MCSimulation(w,p) 
  {
    p_=p;
  }
  
    ///return the Green's function G
  itime_green_function_t get_result() const;
private:
  alps::Parameters p_;
  
};
typedef alps::SimpleRealVectorObservable vec_obs_t;
//vector sincos functions
extern "C" void vdsin_(const int *n, const double *a, double *y);
extern "C" void vdcos_(const int *n, const double *a, double *y);
extern "C" void vdsincos_(const int *n, const double *a, double *s, double *c);
extern "C" void vrda_sincos_(const int *n, const double *a, double *s, double *c);
//extern "C" void dscal_(const unsigned int *size, const double *alpha, double *v, const int *inc);
//
#endif
