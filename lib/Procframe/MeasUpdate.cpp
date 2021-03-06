#pragma ident "$Id: MeasUpdate.cpp $"

/**
 * @file MeasUpdate.cpp
 * Extended Kalman filter process of MeasUpdate.
 */

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
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2009
//
//============================================================================
//
//  Revision
//
//  2014/03/02      Change the 'currentErrorCov' setting: When the 'Variable'
//                  is 'NEW', which can't be found in 'covarianceMap', the
//                  Variance of the variable will be set as the default values,
//                  and the covariance of the variable between other variables
//                  are set 'ZERO'. In the previous version, the variance is
//                  set as '0', and the covariance are set as the default
//                  values, which are not reasonable.
//
//============================================================================


#include "MeasUpdate.hpp"
#include "Counter.hpp"

#ifdef USE_OPENMP
#include <omp.h>
#endif

using namespace std;

namespace gpstk
{

    // Return a string identifying this object.
    std::string MeasUpdate::getClassName() const
    { return "MeasUpdate"; }


    /** Return a reference to a gnssSatTypeValue object after
     *  solving the previously defined equation system.
     *
     * @param gData    Data object holding the data.
     */
    gnssSatTypeValue& MeasUpdate::Process(gnssSatTypeValue& gData)
        throw(ProcessingException)
    {

        try
        {
            // Build a gnssRinex object and fill it with data
            gnssRinex g1;
            g1.header = gData.header;
            g1.body = gData.body;

            // Call the Process() method with the appropriate input object
            Process(g1);

            // Update the original gnssSatTypeValue object with the results
            gData.body = g1.body;

            return gData;
        }
        catch(Exception& u)
        {
            // Throw an exception if something unexpected happens
            ProcessingException e( getClassName() + ":"
                                   + u.what() );

            GPSTK_THROW(e);
        }

    }  // End of method 'MeasUpdate::Process()'



    /** Return a reference to a gnssRinex object after solving
     *  the previously defined equation system.
     *
     * @param gData     Data object holding the data.
     */
    gnssRinex& MeasUpdate::Process(gnssRinex& gData)
        throw(ProcessingException)
    {

        try
        {
            // Build a gnssDataMap object and fill it with data
            gnssDataMap gdsMap;
            SourceID source( gData.header.source );
            gdsMap.addGnssRinex( gData );

            // Call the Process() method with the appropriate input object,
            // and update the original gnssRinex object with the results
            Process(gdsMap);
            gData = gdsMap.getGnssRinex( source );

            return gData;
        }
        catch(Exception& u)
        {
            // Throw an exception if something unexpected happens
            ProcessingException e( getClassName() + ":"
                                   + u.what() );

            GPSTK_THROW(e);
        }

    }  // End of method 'MeasUpdate::Process()'



    /** Return a reference to a gnssDataMap object after solving
     *  the previously defined equation system.
     *
     * @param gData    Data object holding the data.
     */
    gnssDataMap& MeasUpdate::Process( gnssDataMap& gdsMap )
        throw(ProcessingException)
    {

//        cout << "MeasUpdate::Process()" << endl;

        double clk1( Counter::now() );

        int times(0);

        try
        {

            /////////////////////////////////////////////////////////
            //
            // Measurement update using single observables each time.
            //
            /////////////////////////////////////////////////////////

            do
            {
//                cout << "times: " << times << endl;

                // Prepare the equation system with current data
                equSystem.Prepare( gdsMap );

                int numUnknowns( equSystem.getCurrentNumVariables() );

                // Get the set with unknowns to be processed
                VariableSet currentUnknowns( equSystem.getCurrentUnknowns() );

                // Get the list with equations to be processed
                EquationList equList( equSystem.getCurrentEquationsList() );

                int numEquations( equList.size() );

                Vector<double> prefitResiduals( numEquations, 0.0 );
                Matrix<double> hMatrix( numEquations, numUnknowns, 0.0 );

//                cout << "numEquations = " << numEquations << ' '
//                     << "numUnknowns = " << numUnknowns << endl;

                // declare here for memory reuse
                Vector<double> M( numUnknowns, 0.0 );
                Vector<double> K( numUnknowns, 0.0 );

                times++;

                xhat = m_pStateStore->getStateVector( currentUnknowns );
                P = m_pStateStore->getCovarMatrix( currentUnknowns );

//                cout << setprecision(3);
//
//                int iii(0);
//                for(VariableSet::iterator it = currentUnknowns.begin();
//                    it != currentUnknowns.end();
//                    ++it)
//                {
//                    cout << StringUtils::asString(*it)
//                         << setw(10) << xhat(iii)
//                         << setw(20) << P(iii,iii)
//                         << endl;
//
//                    iii++;
//                }


                // Visit each Equation in "equList"
                int row(0);

                for( EquationList::const_iterator itEqu = equList.begin();
                     itEqu != equList.end();
                     ++itEqu )
                {
                    // Get the type value data from the header of the equation
                    typeValueMap tData( (*itEqu).header.typeValueData );

                    // Get the independent type of this equation
                    TypeID indepType( (*itEqu).header.indTerm.getType() );

                    // Now, Let's get current prefit
                    double tempPrefit( tData(indepType) );

                    // Weight
                    double weight( (*itEqu).header.constWeight );

                    // number of Variables in current Equation
                    int numVar( (*itEqu).body.size() );

                    // holding current Equation Variable indexes in Unknowns
                    Vector<int> index(numVar);

                    // holding current Equation Variable coefficients
                    Vector<double> G(numVar);

                    // resize the Matrix and Vector, just reset all element to zero
                    M.resize( M.size(), 0.0 );
                    K.resize( K.size(), 0.0 );


                    // First, fill weight matrix
                    if( indepType == TypeID::prefitC )
                    {
                        if( tData.find(TypeID::weightC) != tData.end() )
                        {
                            weight = weight * tData(TypeID::weightC);
                        }
                    }
                    else if( indepType == TypeID::prefitL )
                    {
                        if( tData.find(TypeID::weightL) != tData.end() )
                        {
                            weight = weight * tData(TypeID::weightL);
                        }
                    }


                    // Second, fill geometry matrix: Look for equation coefficients
                    // Now, let's visit all Variables and the corresponding
                    // coefficients in the equation description
                    int i(0);

//                    cout << indepType << ": " << setw(15) << tempPrefit << "  "
//                         << "number" << ": " << setw(5) << numVar << "  "
//                         << "weight" << ": "<< setw(10) << weight << endl;

                    for( VarCoeffMap::const_iterator vcmIter = (*itEqu).body.begin();
                         vcmIter != (*itEqu).body.end();
                         ++vcmIter )
                    {
                        // We will work with a copy of current Variable
                        Variable var( (*vcmIter).first );

                        // Coefficient Struct
                        Coefficient coef( (*vcmIter).second);

                        // Coefficient values
                        double tempCoef(0.0);

                        // Check if '(*itCol)' unknown variable enforces a specific
                        // coefficient, according the coefficient information from
                        // the equation
                        if( coef.forceDefault )
                        {
                            // Use default coefficient
                            tempCoef = coef.defaultCoefficient;
                        }
                        else
                        {
                            // Look the coefficient in 'tdata'

                            // Get type of current varUnknown
                            TypeID type( var.getType() );

                            // Check if this type has an entry in current GDS type set
                            if( tData.find(type) != tData.end() )
                            {
                                // If type was found, insert value into hMatrix
                                tempCoef = tData(type);
                            }
                            else
                            {
                                // If value for current type is not in gdsMap, then
                                // insert default coefficient for this variable
                                tempCoef = coef.defaultCoefficient;
                            }

                        }  // End of 'if( coef.forceDefault )'

//                        cout << StringUtils::asString(var)
//                             << setw(10) << tempCoef << endl;

                        hMatrix( row, var.getNowIndex() ) = tempCoef;
                        index(i) = var.getNowIndex();
                        G(i) = tempCoef;

                        i++;

                    }  // End of 'for( VarCoeffMap::const_iterator vcmIter = ...'


                    // Now, Let's create the indexes for current float unknowns

                    // Temp measurement
                    double z(tempPrefit);

//                    cout << "Z:" << setw(20) << z << endl;
//                    cout << "G:";
//                    for(int i=0; i<numVar; ++i)
//                    {
//                        cout << setw(10) << G(i);
//                    }
//                    cout << endl;


                    // Inverse weight
                    double inv_W(1.0/weight);


                    // M = P * transpose(G)
                    for(int i=0; i<numUnknowns; i++)
                    {
                        for(int j=0; j<numVar; j++)
                        {
                            M(i) = M(i) + P(i,index(j)) * G(j);
                        }
                    }

                    // dotGM = G * P * transpose(G)
                    double dotGM(0.0);
                    for(int i=0; i<numVar; i++)
                    {
                        dotGM = dotGM + G(i)*M(index(i));
                    }

//                    cout << "dotGM:" << setw(20) << dotGM << endl;

                    // Compute the Kalman gain
                    double beta(inv_W + dotGM);

                    K = M/beta;

//                    cout << "beta:" << setw(20) << beta << endl;

//                    cout << "K:" << endl;
//                    for(int i=0; i<numUnknowns; ++i)
//                    {
//                        if(K(i) != 0)
//                        cout << setw(10) << K(i) << endl;
//                    }

                    double dotGX(0.0);
                    for(int i=0; i<numVar; i++)
                    {
                        dotGX = dotGX + G(i)*xhat(index(i));
                    }

//                    cout << "dotGX:" << setw(20) << dotGX << endl;

                    // State update
                    xhat = xhat + K*( z - dotGX );

//                    cout << "xhat:" << endl;
//                    for(int i=0; i<numUnknowns; ++i)
//                    {
//                        if(xhat(i) != 0)
//                        cout << setw(10) << xhat(i) << endl;
//                    }


                    // Covariance update
                    // old version:
                    // P = P - outer(K,M);
                    // Considering that the P and KM matrix are symmetric,
                    // thus the computation can be accelerated by operating
                    // the upper triangular matrix.
#ifdef _OPENMP
   #pragma omp parallel for
#endif
                    for(int i=0;i<numUnknowns;i++)
                    {
                        // The diagonal element
                        P(i,i) = P(i,i) -  K(i)*M(i);

                        // The upper/lower triangular element
                        for(int j=(i+1);j<numUnknowns;j++)
                        {
                            P(j,i) = P(i,j) = P(i,j) - K(i)*M(j);
                        }

                    }  // End of 'for(int i = 0; ...)'

                    // insert current 'prefit' into 'prefitResiduals'
                    prefitResiduals(row) = tempPrefit;

                    // Increment row number
                    row++;

                }  // End of 'for( EquationList::const_iterator itEqu = ...'

                // Compute the postfit residuals Vector
                postfitResiduals = prefitResiduals - (hMatrix* xhat);

//                for(int i=0; i<numEquations; ++i)
//                {
//                    cout << setw(10) << postfitResiduals(i) << endl;
//                }

//                cout << "hMatrix:" << endl;
//                for(int i=0; i<numEquations; ++i)
//                {
//                    for(int j=0; j<numUnknowns; ++j)
//                    {
//                        if(hMatrix(i,j) != 0)
//                            cout << setw(5) << i
//                                 << setw(5) << j
//                                 << setw(10) << hMatrix(i,j);
//                    }
//                    cout << endl;
//                }
//
//                cout << "xhat:" << endl;
//                for(int i=0; i<numUnknowns; ++i)
//                {
//                    if(xhat(i) != 0)
//                    cout << setw(10) << xhat(i) << endl;
//                }

//           } while( !postfitFilter( gdsMap ) && times < 3);
            } while( false );

            //////////// //////////// //////////// ////////////
            //
            //  postCompute, store the xhat and P into stateMap
            //  and covarianceMap for the next epoch.
            //
            //////////// //////////// //////////// ////////////

            m_pStateStore->setStateVector( xhat );
            m_pStateStore->setCovarMatrix( P );
            m_pStateStore->setVariableSet( equSystem.getCurrentUnknowns() );

        }
        catch(Exception& u)
        {
            // Throw an exception if something unexpected happens
            ProcessingException e( getClassName() + ":" + u.what() );
            std::cout << "exception: " << e.what() << std::endl;

            GPSTK_THROW(e);
        }

        double clk2( Counter::now() );

//        cout << "time ellapsed: " << clk2-clk1 << endl;

        return gdsMap;

    }  // End of method 'MeasUpdate::Process()'


    /// Postfit filter.
    bool MeasUpdate::postfitFilter(gnssDataMap& gdsMap)
    {
        // get equation list
        EquationList equList = equSystem.getCurrentEquationsList();

        double sigma = 0.0;
        bool isValid = true;

        int i = 0;
        for( EquationList::iterator itEqu = equList.begin();
             itEqu != equList.end();
             ++itEqu )
        {
            TypeID residualType( (*itEqu).header.indTerm.getType() );
            typeValueMap tvm( (*itEqu).header.typeValueData );

            double weight( (*itEqu).header.constWeight );

            if( residualType == TypeID::prefitC )
            {
                weight = weight * tvm(TypeID::weightC);
            }
            else if( residualType == TypeID::prefitL )
            {
                weight = weight * tvm(TypeID::weightL);
            }

            sigma += std::pow( postfitResiduals(i), 2 ) * weight;

            i++;
        }

        int n =  equList.size();
        int t =  equSystem.getCurrentNumVariables();
        sigma = std::sqrt( sigma/(n-t) );

//        cout << "sigma: " << sigma << endl;

        SourceID sourceRemoved;
        SatID satRemoved;

        i = 0;
        for( EquationList::iterator itEqu = equList.begin();
             itEqu != equList.end();
             ++itEqu )
        {
            SourceID source( (*itEqu).header.equationSource );
            SatID sat( (*itEqu).header.equationSat );
            TypeID residualType( (*itEqu).header.indTerm.getType() );

            double weight( (*itEqu).header.constWeight );
            typeValueMap& tvm( (*itEqu).header.typeValueData );

            if( residualType == TypeID::prefitC )
            {
                weight = weight * tvm(TypeID::weightC);
            }
            else if( residualType == TypeID::prefitL )
            {
                weight = weight * tvm(TypeID::weightL);
            }

            double v( std::sqrt(weight) * std::fabs(postfitResiduals(i)) );

//            if(v > 1.5*sigma)
//            {
//                cout << setw(10) << source.sourceName
//                     << setw(10) << sat;
//                cout << setw(20) << residualType;
//                cout << setw(10) << v
//                     << setw(10) << 1.5*sigma/v
//                     << setw(10) << gdsMap.getValue(source,sat,TypeID::weightC)
//                     << setw(10) << gdsMap.getValue(source,sat,TypeID::weightL)
//                     << endl;
//            }

//            if( (v>1.5*sigma) && (v<=2.5*sigma) )
//            {
//                isValid = false;
//
//                weight = weight * 1.5*sigma/v;
//
//                if(residualType == TypeID::prefitC)
//                {
//                    if( !((source==sourceRemoved) && (sat==satRemoved)) )
//                    {
//                        gdsMap.insertValue(source,sat,TypeID::weightC, weight);
//                    }
//                }
//                else if(residualType == TypeID::prefitL)
//                {
//                    if( !((source==sourceRemoved) && (sat==satRemoved)) )
//                    {
//                        gdsMap.insertValue(source,sat,TypeID::weightL, weight);
//                    }
//                }
//            }
//            else if( v > 2.5*sigma )
            if( v > 2.5*sigma )
            {
                isValid = false;

                sourceRemoved = source;
                satRemoved = sat;

                gdsMap.removeSatID(source, sat);
            }

            i++;
        }


        for( gnssDataMap::iterator gdsIter = gdsMap.begin();
             gdsIter != gdsMap.end(); )
        {
            for( sourceDataMap::iterator sdmIter = gdsIter->second.begin();
                 sdmIter != gdsIter->second.end(); )
            {
                if( sdmIter->second.numSats() <= 3 )
                {
                    gdsIter->second.erase(sdmIter++);
                }
                else
                {
                    sdmIter++;
                }
            }

            if( gdsIter->second.size() <= 0 )
            {
                gdsMap.erase(gdsIter++);
            }
            else
            {
                gdsIter++;
            }
        }


        return isValid;

    } // End of method 'MeasUpdate::postfitFilter()'


}  // End of namespace gpstk
