/*****************************************************************************
*   Copyright (C) 2004-2009 The PaGMO development team,                     *
*   Advanced Concepts Team (ACT), European Space Agency (ESA)               *
*   http://apps.sourceforge.net/mediawiki/pagmo                             *
*   http://apps.sourceforge.net/mediawiki/pagmo/index.php?title=Developers  *
*   http://apps.sourceforge.net/mediawiki/pagmo/index.php?title=Credits     *
*   act@esa.int                                                             *
*                                                                           *
*   This program is free software; you can redistribute it and/or modify    *
*   it under the terms of the GNU General Public License as published by    *
*   the Free Software Foundation; either version 2 of the License, or       *
*   (at your option) any later version.                                     *
*                                                                           *
*   This program is distributed in the hope that it will be useful,         *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
*   GNU General Public License for more details.                            *
*                                                                           *
*   You should have received a copy of the GNU General Public License       *
*   along with this program; if not, write to the                           *
*   Free Software Foundation, Inc.,                                         *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.               *
*****************************************************************************/

#ifndef LEG_H
#define LEG_H

#include<vector>
#include"../spacecraft.h"
#include"../core_functions/array3D_operations.h"
#include"../core_functions/propagate_lagrangian.h"
#include"sc_state.h"
#include"../epoch.h"
#include "throttle.h"
#include "../exceptions.h"

namespace kep_toolbox {
/// Objects dealing with the Sims-Flanagan low-thrust trajectory model
/**
 * This namespace contains the routines that allow building and evaluating low-thrust trajectories using the
 * Sims-Flanagan transcription method.
 */
    namespace sims_flanagan{

/// A generic trajectory low-thrust leg as represented by a series of impulsive manouvres (Sims and Flanagan model)
/**
 * This class represents, generically, a low-thrust trajectory leg as a sequence of successive
 * impulses of magnitude compatible with the low-thrust propulsion system of a spacecraft.
 * The leg achieves to transfer a given spacecraft from an initial to a final state in the
 * time given (and is thus defined feasible) whenever the method evaluate_mismatch
 * returns all zeros and the method get_throttles_con returns all values less than zero.
 * The different impulses are represented by 'throttles'. These represent
 * the cartesian components \f$ \mathbf x = (x_1,y_1,z_1) \f$ of normalized \f$ \Delta V \f$ and are thus
 * numbers in the range \f$ \in [0,1] \f$ that need to satisfy the constraint \f$|\mathbf x| \le 1\f$
 *
 * \image html sims_flanagan_leg.png "Visualization of a feasible leg (Earth-Mars)"
 * \image latex sims_flanagan_leg.png "Visualization of a feasible leg (Earth-Mars)" width=5cm
 *
 * @author Dario Izzo (dario.izzo _AT_ googlemail.com)
 */
	class leg
	{
	    friend std::ostream &operator<<(std::ostream &s, const leg &in );

	public:
	    /// Constructor.
	    /**
	     * Constructs a leg without initialising any of its members. This constructor needs to
	     * be followed by a call to the setters to initialize all of its contents and is provided only
	     * to allow other classes to call a default constructor for this class.
	     */
	    leg() {}

	    /** @name Setters*/
	    //@{
	    template<typename it_type>
	    void set_leg(const epoch& epoch_i, const sc_state& state_i,
			 it_type throttles_start,
			 it_type throttles_end,
			 const epoch& epoch_f, const sc_state& state_f,
			 double mu_)
	    {
		if (epoch_f.mjd2000() <= epoch_i.mjd2000()) {
		    throw_value_error("Final epoch is before initial epoch");
		}
		
		t_i = epoch_i; x_i=state_i;
		t_f = epoch_f; x_f=state_f;
		
		throttles.assign(throttles_start, throttles_end);
		
		if (mu_<=0) {
		    throw_value_error("Gravitational constant is less or equal to zero");
		}
		mu = mu_;
	    }
	    
	    /// Sets the leg's spacecraft
	    /**
	     *
	     * In order for the trajectory leg to be able to propagate the states, information on the
	     * low-thrust propulsion system used needs to be available. This is provided by the object
	     * spacecraft private member of the class and can be set using this setter.
	     *
	     * \param[in] sc_ The spacecraft object
	     */
	    void set_spacecraft(const spacecraft &sc_) { sc = sc_; }
	    
	    const spacecraft& get_spacecraft() const { return sc; }
	    
	    /// Sets the leg's central body gravitational parameter
	    /**
	     *
	     * Sets the leg's central body gravitational parameter
	     *
	     * \param[in] mu_ The gravitational parameter
	     */
	    void set_mu(const double &mu_) { mu = mu_; }

	    /// Sets the throttles
	    /**
	     *
	     * Sets the std:vector containing all the leg throttles.
	     *
	     * \param[in] throttles_ STL vector containing the throttles in the format \f$ (x_1,y_1,z_1,x_2,y_2,z_2,...,x_n,y_n,z_n) \f$
	     * where \f$ n \f$ is the number of segments.
	     */
	    template<typename it_type>
	    void set_throttles(it_type b, it_type e) { throttles.assign(b, e); }
	    
	    /// Sets the throttles size
	    /**
	     * Resizes the throttles vector to a new size.
	     *
	     * \param[in] size The new size of the throttles. Needs to be a multiple of 3
	     */
	    void set_throttles_size(const int& size) { throttles.resize(size); }

	    /**
	     * Sets the ith throttle
	     *
	     * \param[in] index the index of the throttle
	     * \param[in] t the throttle
	     */
	    void set_throttle(int index, const throttle& t) { throttles[index] = t; }
	    //@}

	    /** @name Getters*/
	    //@{
	    /// Gets the throttle vector size
	    /**
	     * Returns the throttle vector size.
	     *
	     * @return size_t containing the throttle vector size.
	     */
	    size_t get_throttles_size() const {return throttles.size();}

	    const throttle& get_throttle(int index) { return throttles[index]; }
	    
	    /// Gets the initial epoch
	    /**
	     * Gets the epoch at the beginning of the leg
	     *
	     * @return const reference to the initial epoch
	     */
	    epoch get_t_i() const {return t_i;}

	    /// Gets the final epoch
	    /**
	     * Gets the epoch at the end of the leg
	     *
	     * @return const reference to the final epoch
	     */
	    epoch get_t_f() const {return t_f;}

	    /// Sets the initial epoch
	    /**
	     * Sets the epoch at the beginning of the leg
	     *
	     */
	    void set_t_i(epoch e) { t_i = e; }

	    /// Sets the final epoch
	    /**
	     * Sets the epoch at the end of the leg
	     *
	     */
	    void set_t_f(epoch e) { t_f = e; }

	    /// Gets the final sc_state
	    /**
	     * Gets the spacecraft state at the end of the leg
	     *
	     * @return const reference to the final sc_state
	     */
	    const sc_state& get_x_f() const {return x_f;}

	    /// Gets the initial sc_state
	    /**
	     * Gets the spacecraft state at the beginning of the leg
	     *
	     * @return const reference to the initial sc_state
	     */
	    const sc_state& get_x_i() const {return x_i;}
	    
	    /// Sets the final sc_state
	    /**
	     * Sets the spacecraft state at the end of the leg
	     *
	     */
	    void set_x_f(const sc_state& s) { x_f = s;}

	    /// Sets the initial sc_state
	    /**
	     * Sets the spacecraft state at the beginning of the leg
	     *
	     */
	    void set_x_i(const sc_state& s) { x_i = s;}
	    //@}

	    /** @name Leg Feasibility Checks*/
	    //@{

	    /// Evaluate the state mismatch
	    /**
	     * This is the main method of the class leg as it performs the orbital propagation from the initial sc_state, and
	     * accounting for all the throttles, up to a mid-point. The same is done starting from the final sc_state up to
	     * the same mid-point. The difference between the obtained values is then returned. If not all zero the leg
	     * is unfeasible.
	     *
	     * @param[out] mismatch sc_state containing the state mismatch at the mid-point
	     */
	    template<typename it_type>
	    void get_mismatch_con(it_type begin, it_type end) const
	    {
		assert(end - begin == 7);
		size_t n_seg = throttles.size();
		const int n_seg_fwd = (n_seg + 1) / 2, n_seg_back = n_seg / 2;

		//Aux variables
		double max_thrust = sc.get_thrust();
		double isp = sc.get_isp();
		double norm_dv;
		array3D dv;

		//Initial state
		array3D rfwd = x_i.get_position();
		array3D vfwd = x_i.get_velocity();
		double mfwd = x_i.get_mass();

		//Forward Propagation
		double current_time_fwd = t_i.mjd2000() * ASTRO_DAY2SEC;
		for (int i = 0; i < n_seg_fwd; i++) {
		    double thrust_duration = (throttles[i].get_end().mjd2000() -
					      throttles[i].get_start().mjd2000()) * ASTRO_DAY2SEC;
		    double manouver_time = (throttles[i].get_start().mjd2000() +
					    throttles[i].get_end().mjd2000()) / 2. * ASTRO_DAY2SEC;
		    propagate_lagrangian(rfwd, vfwd, manouver_time - current_time_fwd, mu);
		    current_time_fwd = manouver_time;
		    
		    for (int j=0;j<3;j++)
			dv[j] = max_thrust / mfwd * thrust_duration * throttles[i].get_value()[j];
		    norm_dv = norm(dv);
		    sum(vfwd,vfwd,dv);
		    mfwd *= exp( -norm_dv/isp/ASTRO_G0 );
		}
		
		//Final state
		array3D rback = x_f.get_position();
		array3D vback = x_f.get_velocity();
		double mback = x_f.get_mass();
		
		//Backward Propagation
		double current_time_back = t_f.mjd2000() * ASTRO_DAY2SEC;
		for (int i = 0; i < n_seg_back; i++) {
		    double thrust_duration = (throttles[throttles.size() - i - 1].get_end().mjd2000() -
					      throttles[throttles.size() - i - 1].get_start().mjd2000()) * ASTRO_DAY2SEC;
		    double manouver_time = (throttles[throttles.size() - i - 1].get_start().mjd2000() +
					    throttles[throttles.size() - i - 1].get_end().mjd2000()) / 2. * ASTRO_DAY2SEC;
		    // manouver_time - current_time_back is negative, so this should propagate backwards
		    propagate_lagrangian(rback, vback, manouver_time - current_time_back, mu);
		    current_time_back = manouver_time;
		    
		    for (int j=0;j<3;j++)
			dv[j] = - max_thrust / mback * thrust_duration * throttles[throttles.size() - i - 1].get_value()[j];
		    norm_dv = norm(dv);
		    sum(vback,vback,dv);
		    mback *= exp( norm_dv/isp/ASTRO_G0 );
		}
		
		// finally, we propagate from current_time_fwd to current_time_back with a keplerian motion
		propagate_lagrangian(rfwd, vfwd, current_time_back - current_time_fwd, mu);
		
		//Return the mismatch
		diff(rfwd,rfwd,rback);
		diff(vfwd,vfwd,vback);

		std::copy(rfwd.begin(), rfwd.end(), begin);
		std::copy(vfwd.begin(), vfwd.end(), begin + 3);
		begin[6] = mfwd - mback;
	    }

	    void get_mismatch_con(sc_state retval) const
	    {
		array7D tmp;
		get_mismatch_con(tmp.begin(), tmp.end());
		retval.set_state(tmp);
	    }

		//TODO: document me and pay attention to my name, I do not evaluate the real dv, I am a joke (so what?)
	    double evaluate_dv() const
	    {
		double tmp = 0;
		for (int i = 0; i < throttles.size(); ++i) {
			tmp += (throttles[i].get_end().mjd2000() -throttles[i].get_start().mjd2000())
			* ASTRO_DAY2SEC * throttles[i].get_norm() * sc.get_thrust() / sc.get_mass();
		}
		return tmp;
	}
		

	/// Evaluate the throttles magnitude
	/**
	 * This methods loops on the vector containing the throttles \f$ (x_1,y_1,z_1,x_2,y_2,z_2,...,x_n,y_n,z_n) \f$
	 * and stores the magnitudes \f$ x_i^2 + y_i^2 + z_i^2 - 1\f$ at the locations indicated by the iterators. The
	 * iterators must have a distance of \f$ n\f$. If the stored values are not all \f$ \le 1\f$ then the trajectory
	 * is unfeasible.
	 *
	 * @param[out] start std::vector<double>iterator from the first element where to store the magnitudes
	 * @param[out] start std::vector<double>iterator to the last+1 element where to store the magnitudes
	 */
	template<typename it_type>
	void get_throttles_con(it_type start, it_type end) const {
	    if ( (end - start) != (int)throttles.size()) {
		throw_value_error("Iterators distance is incompatible with the throttles size");
	    }
	    int i=0;
	    while(start!=end){
		const array3D& t = throttles[i].get_value();
		*start = std::inner_product(t.begin(), t.end(), t.begin(), -1.);
		++i; ++start;
	    }
	}
	    //@}
	    
	private:
	    epoch t_i;
	    sc_state x_i;
	    std::vector<throttle> throttles;
	    epoch t_f;
	    sc_state x_f;
	    spacecraft sc;
	    double mu;
	};

	std::ostream &operator<<(std::ostream &s, const leg &in );

    }} //namespaces
#endif // LEG_H
