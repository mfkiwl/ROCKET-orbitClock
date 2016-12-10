/**
 * @file ReferenceSystem.hpp
 * This class ease handling reference system transformation.
 */

#ifndef REFERENCE_SYSTEM_HPP
#define REFERENCE_SYSTEM_HPP

//============================================================================
//
//  This file is part of GPSTk, the GPS Toolkit.
//
//  The GPSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 2.1 of the License, or
//  any later version.
//
//  The GPSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GPSTk; if not, write to the Free Software Foundation,
//  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Last Modified:
//
//  2013/05/29   Create this file
//
//  Shoujian Zhang, Wuhan University
//
//============================================================================

#include <string>
#include "GNSSconstants.hpp"
#include "Vector.hpp"
#include "Matrix.hpp"
#include "CommonTime.hpp"
#include "EOPDataStore2.hpp"
#include "LeapSecStore.hpp"
#include "Exception.hpp"

namespace gpstk
{

    /** @addtogroup GeoDynamics */
    //@{

    /** Class to handle the reference system transformation, i.e.
     *  Transforming the ICRS to ITRS and vice versa.
     *
     *  All the transformation matrix are computed using the
     *  analytic fomulas.
     *
     *  The Earth Orentation Parameters are from the EOPDataStore2,
     *  and the leap seconds data are from LeapSecStore.
     *
     *  When convert the ICRS to ITRS, you will use the time system
     *  conversion, and the relation between different time system
     *  follows as:
     *
     *           -14s
     *     -----------------> BDT(Compass Time)
     *     |
     *     |         +19s             +32.184s           +rel.effects
     *    GPS -------------> TAI ----------------> TT -----------------> TDB
     *                       T |
     *            -(UT1-TAI) | |    -leap seconds
     *    UT1 ---------------| |--------------------> UTC
     *     |
     *     |   earth rotation
     *     ---------------------> GAST
     *
     *  Most of the methods are modified from the code provided by
     *  Montenbruck, Oliver, 2001, as the attachement of the book
     *  "Satellite Orbits: Models, Methods and applications".
     *
     */
    class ReferenceSystem
    {
    public:

        /// Default constructor.
        ReferenceSystem()
            : pEopStore(NULL), pLeapSecStore(NULL), isPrepared(false)
        {}


        /** Constructor.
         */
        ReferenceSystem(EOPDataStore2& eopStore,
                        LeapSecStore& leapSecStore)
            : isPrepared(false)
        {
            pEopStore = &eopStore;
            pLeapSecStore = &leapSecStore;
        }


        /// Set the EOP data store.
        ReferenceSystem& setEOPDataStore(EOPDataStore2& eopStore)
        { pEopStore = &eopStore; return (*this); };


        /// Get the EOP data store.
        EOPDataStore2* getEOPDataStore() const
        { return pEopStore; };


        /// Set the leapsec data store.
        ReferenceSystem& setLeapSecStore(LeapSecStore& leapSecStore)
        { pLeapSecStore = &leapSecStore; return (*this); };


        /// Get the leapsec data store.
        LeapSecStore* getLeapSecStore() const
        { return pLeapSecStore; };


        /// Get the x pole displacement parameter, in arcseconds
        double getXPole(const CommonTime& UTC) const
            throw (InvalidRequest);


        /// Get the y pole displacement parameter, in arcseconds
        double getYPole(const CommonTime& UTC) const
            throw (InvalidRequest);


        /// Get the value of (UT1 - UTC), in seconds
        double getUT1mUTC(const CommonTime& UTC) const
            throw (InvalidRequest);

        double getLOD(const CommonTime& UTC) const
            throw(InvalidRequest);

        /// Get the value of dX, in arcseconds
        double getDX(const CommonTime& UTC) const
            throw(InvalidRequest);

        /// Get the value of dY, in arcseconds
        double getDY(const CommonTime& UTC) const
            throw(InvalidRequest);


        /// Get the EOP data at the given UTC time.
        EOPDataStore2::EOPData getEOPData(const CommonTime& UTC) const
            throw(InvalidRequest);


        /// Get the leapsec data at the given UTC time.
        double getLeapSec(const CommonTime& UTC) const
            throw(InvalidRequest);


        ///------ Methods to get the difference of different time systems ------//


        /// Get the value of (TAI - UTC) (= leapsec).
        double getTAImUTC(const CommonTime& UTC)
            throw(InvalidRequest);


        /// Get the value of (TT - UTC) (= 32.184 + leapsec).
        double getTTmUTC(const CommonTime& UTC)
            throw(InvalidRequest);


        /// Get the value of (TT - TAI) (= 32.184s).
        double getTTmTAI(void)
            throw(InvalidRequest);


        /// Get the value of (TAI - GPS) (= -19s).
        double getTAImGPS(void)
            throw(InvalidRequest);



        ///------ Methods to convert between different time systems ------//


        /// Convert GPS to UTC.
        CommonTime GPS2UTC(const CommonTime& GPS);


        /// Convert UTC to GPS.
        CommonTime UTC2GPS(const CommonTime& UTC);


        /// Convert UT1 to UTC.
        CommonTime UT12UTC(const CommonTime& UT1);


        /// Convert UTC to UT1.
        CommonTime UTC2UT1(const CommonTime& UTC);
        CommonTime UTC2UT1(const CommonTime& UTC, const double& UT1mUTC);


        /// Convert TAI to UTC.
        CommonTime TAI2UTC(const CommonTime& TAI);


        /// Convert UTC to TAI.
        CommonTime UTC2TAI(const CommonTime& UTC);


        /// Convert TT to UTC.
        CommonTime TT2UTC(const CommonTime& TT);


        /// Convert UTC to TT.
        CommonTime UTC2TT(const CommonTime& UTC);



        /// Convert TDB to UTC.
        CommonTime TDB2UTC(const CommonTime& TDB);


        /// Convert UTC to TDB.
        CommonTime UTC2TDB(const CommonTime& UTC);



        ///------ Methods to do reference system transformation ------//


        /**Mean obliquity of the ecliptic, IAU 2006 precession model.
         *
         * @param   TT
         * @return  Mean obliquity of the ecliptic
         */
        double MeanObliquity06(const CommonTime& TT);


        /**X,Y coordinates of celestial intermediate pole from series based
         * on IAU 2006 precession and IAU 2000A nutation.
         *
         * @param  TT
         * @return x,y     CIP X,Y coordinates
         */
        void XY06(const CommonTime& TT, double& X, double& Y);


        /**The CIO locator s, positioning the Celestial Intermediate Origin on
         * the equator of the Celestial Intermediate Pole, given the CIP's X,Y
         * coordinates. Compatible with IAU 2006/2000A precession-nutation.
         *
         * @param  TT
         *         x,y     CIP coordinates
         * @return the CIO locator s in radians
         */
        double S06(const CommonTime& TT, const double& X, const double& Y);


        /**Form the celestial to intermediate-frame-of-date matrix given the CIP
         * X,Y and the CIO locator s.
         *
         * @param  x,y     Celestial Intermediate Pole
         *         s       the CIO locator s
         * @return celestial-to-intermediate matrix
         */
        Matrix<double> C2IXYS(const double& X, const double& Y, const double& s);


        /**Earth rotation angle (IAU 2000 model).
         *
         * @param  UT1
         * @return Earth rotation angle (radians), range 0-2pi
         */
        double ERA00(const CommonTime& UT1);


        /**The TIO locator s', positioning the Terrestrial Intermediate Origin
         * on the equator of the Celestial Intermediate Pole.
         *
         * @param   TT
         * @return  the TIO locator s' in radians
         */
        double SP00(const CommonTime& TT);


        /**Form the matrix of polar motion for a given date, IAU 2000.
         *
         * @param  xp,yp   coordinates of the pole (radians)
         *         sp      the TIO locator s' (radians)
         * @return polar-motion matrix
         */
        Matrix<double> POM00(const double& xp, const double& yp, const double& sp);



        /// Transformation matrix from CRS to TRS coordinates for a given date
        Matrix<double> C2TMatrix(const CommonTime& UTC);


        /// Transformation matrix form TRS to CRS coordinates for a given date
        Matrix<double> T2CMatrix(const CommonTime& UTC);


        /// Earth rotation angle first order rate
        double dERA00(const CommonTime& UT1);

        /// Time derivative of transformation matrix from CRS to TRS coordinates
        /// for a given date
        Matrix<double> dC2TMatrix(const CommonTime& UTC);


        /// Time derivative of transformation matrix from TRS to CRS coordinates
        /// for a given date
        Matrix<double> dT2CMatrix(const CommonTime& UTC);


        /**Greenwich mean sidereal time (consistent with IAU 2006 precession).
         *
         * @param  UT1
         *         TT
         * @return Greenwich mean sidereal time (radians)
         */
        double GMST06(const CommonTime& UT1, const CommonTime& TT);


        /// Compute doodson's fundamental arguments (BETA)
        /// and fundamental arguments for nutation (FNUT)
        void DoodsonArguments(const  CommonTime& UT1,
                              const  CommonTime& TT,
                              double BETA[6],
                              double FNUT[5]         );



        /**Convert coordinate difference in XYZ to RTN.
         *
         * @param dxyz     Coordinate difference in XYZ
         * @param r        Position used to define RTN
         * @param v        Velocity used to define RTN
         * @return         Coordinate difference in RTN
         */
        Vector<double> XYZ2RTN(const Vector<double>& dxyz,
                               const Vector<double>& r,
                               const Vector<double>& v);


        /**Fundamental argument, IERS Conventions (2003):
         * mean anomaly of the Moon.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      l, mean anomaly of the Moon (radians)
         */
        inline double Fal03(const double& t)
        {
            double a;

            /* Mean anomaly of the Moon (IERS Conventions 2003). */
            a = std::fmod(           485868.249036  +
                            t * ( 1717915923.2178 +
                            t * (         31.8792 +
                            t * (          0.051635 +
                            t * (        - 0.00024470 ) ) ) ), TURNAS ) * AS_TO_RAD;

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean anomaly of the Sun.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      l', radians
         */
        inline double Falp03(const double& t)
        {
            double a;

            /* Mean anomaly of the Sun (IERS Conventions 2003). */
            a = std::fmod(         1287104.793048 +
                            t * ( 129596581.0481 +
                            t * (       - 0.5532 +
                            t * (         0.000136 +
                            t * (       - 0.00001149 ) ) ) ), TURNAS ) * AS_TO_RAD;

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean longitude of the Moon minus mean longitude of the ascending
         * node.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      F, radians
         */
        inline double Faf03(const double& t)
        {
            double a;

            /* Mean longitude of the Moon minus that of the ascending node */
            /* (IERS Conventions 2003).                                    */
            a = std::fmod(           335779.526232 +
                            t * ( 1739527262.8478 +
                            t * (       - 12.7512 +
                            t * (        - 0.001037 +
                            t * (          0.00000417 ) ) ) ), TURNAS ) * AS_TO_RAD;

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean elongation of the Moon from the Sun.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      D, radians
         */
        inline double Fad03(const double& t)
        {
            double a;

            /* Mean elongation of the Moon from the Sun (IERS Conventions 2003). */
            a = std::fmod(          1072260.703692 +
                            t * ( 1602961601.2090 +
                            t * (        - 6.3706 +
                            t * (          0.006593 +
                            t * (        - 0.00003169 ) ) ) ), TURNAS ) * AS_TO_RAD;

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean longitude of the Moon's ascending node.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      Omega, radians
         */
        inline double Faom03(const double& t)
        {
            double a;

            /* Mean longitude of the Moon's ascending node */
            /* (IERS Conventions 2003).                    */
            a = std::fmod(          450160.398036 +
                            t * ( - 6962890.5431 +
                            t * (         7.4722 +
                            t * (         0.007702 +
                            t * (       - 0.00005939 ) ) ) ), TURNAS ) * AS_TO_RAD;

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean longitude of Mercury.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      mean longitude of Mercury, radians
         */
        inline double Fame03(const double& t)
        {
            double a;

            /* Mean longitude of Mercury (IERS Conventions 2003). */
            a = std::fmod(4.402608842 + 2608.7903141574 * t, TWO_PI);

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean longitude of Venus.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      mean longitude of Venus, radians
         */
        inline double Fave03(const double& t)
        {
            double a;

            /* Mean longitude of Venus (IERS Conventions 2003). */
            a = std::fmod(3.176146697 + 1021.3285546211 * t, TWO_PI);

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean longitude of Earth.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      mean longitude of Earth, radians
         */
        inline double Fae03(const double& t)
        {
            double a;

            /* Mean longitude of Earth (IERS Conventions 2003). */
            a = std::fmod(1.753470314 + 628.3075849991 * t, TWO_PI);

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean longitude of Mars.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      mean longitude of Mars, radians
         */
        inline double Fama03(const double& t)
        {
            double a;

            /* Mean longitude of Mars (IERS Conventions 2003). */
            a = std::fmod(6.203480913 + 334.0612426700 * t, TWO_PI);

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean longitude of Jupiter.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      mean longitude of Jupiter, radians
         */
        inline double Faju03(const double& t)
        {
            double a;

            /* Mean longitude of Jupiter (IERS Conventions 2003). */
            a = std::fmod(0.599546497 + 52.9690962641 * t, TWO_PI);

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean longitude of the Saturn.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      mean longitude of Saturn, radians
         */
        inline double Fasa03(const double& t)
        {
            double a;

            /* Mean longitude of Saturn (IERS Conventions 2003). */
            a = std::fmod(0.874016757 + 21.3299104960 * t, TWO_PI);

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean longitude of Uranus.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      mean longitude of Uranus, radians
         */
        inline double Faur03(const double& t)
        {
            double a;

            /* Mean longitude of Uranus (IERS Conventions 2003). */
            a = std::fmod(5.481293872 + 7.4781598567 * t, TWO_PI);

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * mean longitude of Neptune.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      mean longitude of Neptune, radians
         */
        inline double Fane03(const double& t)
        {
            double a;

            /* Mean longitude of Neptune (IERS Conventions 2003). */
            a = std::fmod(5.311886287 + 3.8133035638 * t, TWO_PI);

            return a;
        }


        /**Fundamental argument, IERS Conventions (2003):
         * general accumulated precession in longitude.
         *
         * @param t     TDB, Julian centuries since J2000.0
         * @return      general precession in longitude, radians
         */
        inline double Fapa03(const double& t)
        {
            double a;

            /* General accumulated precession in longitude. */
            a = (0.024381750 + 0.00000538691 * t) * t;

            return a;
        }


        /**Normalize angle into the range -PI <= a <= +PI.
         *
         * @param  a    Angle (radians)
         * @return      Angle in range +/-PI
         */
        double Anpm(double a)
        {
            double w = std::fmod(a, TWO_PI);
            if(fabs(w) >= PI)
            {
                if(a > 0.0)   w -=  TWO_PI;
                else          w -= -TWO_PI;
            }

            return w;
        }


        /**Normalize angle into the range 0 <= a <= 2PI.
         *
         * @param  a    Angle (radians)
         * @return      Angle in range 0-2PI
         */
        double Anp(double a)
        {
            double w = std::fmod(a, TWO_PI);

            if(w < 0.0)
            {
                w += TWO_PI;
            }

            return w;
        }


        /// Deconstrutor
        virtual ~ReferenceSystem() {};


    private:

        /// Pointer to the EOPDataStore
        EOPDataStore2* pEopStore;

        /// Pointer to the leap second store
        LeapSecStore* pLeapSecStore;

        /// whether the transformation matrix is prepared
        bool isPrepared;

    }; // End of class 'ReferenceSystem'

    // @}

}  // End of namespace 'gpstk'

#endif  // REFERENCE_SYSTEM_HPP
