/**
 * Copyright (c) 2006 Benjamin C. Meyer (ben at meyerhome dot net)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Benjamin Meyer nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
//=============================================================================
//
// SVD Sample Code
//
// Copyright (C) 2007 Timely Development (www.timelydevelopment.com)
//
// Special thanks to Simon Funk and others from the Netflix Prize contest 
// for providing pseudo-code and tuning hints.
//
// Feel free to use this code as you wish as long as you include 
// these notices and attribution. 
//
// Also, if you have alternative types of algorithms for accomplishing 
// the same goal and would like to contribute, please share them as well :)
//
// STANDARD DISCLAIMER:
//
// - THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY
// - OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT
// - LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR
// - FITNESS FOR A PARTICULAR PURPOSE.
//
//=============================================================================
*/

#include <probe.h>
#include <user.h>

#define MAX_RATINGS     100480508     // Ratings in entire training set (+1)
#define MAX_CUSTOMERS   480190        // Customers in the entire training set (+1)
#define MAX_MOVIES      17771         // Movies in the entire training set (+1)
#define MAX_FEATURES    64            // Number of features to use 
//#define MIN_EPOCHS      120           // Minimum number of epochs per feature
//#define MAX_EPOCHS      200           // Max epochs per feature
#define MIN_EPOCHS      2           // Minimum number of epochs per feature
#define MAX_EPOCHS      5           // Max epochs per feature

#define MIN_IMPROVEMENT 0.0001        // Minimum improvement required to continue current feature
#define INIT            0.1           // Initialization value for features
#define LRATE           0.001         // Learning rate parameter
#define K               0.015         // Regularization parameter used to minimize over-fitting

class Engine : public Algorithm
{

public:
    Engine(DataBase *db);
    ~Engine(void) { };

    void calcFeatures();

    void setMovie(int id)
    {
        currentMovie.setId(id);
    }

    double determine(int user);

private:
    float           m_aCustFeatures[MAX_FEATURES][MAX_CUSTOMERS];   // Array of features by customer (using floats to save space)
    float           m_aMovieFeatures[MAX_FEATURES][MAX_MOVIES];     // Array of features by movie (using floats to save space)
    float           cache[MAX_RATINGS];
    Movie currentMovie;

    inline double predictRating(short movieId, int custId, int feature, float cache, bool bTrailing=true) const;
};

//-------------------------------------------------------------------
// Initialization
//-------------------------------------------------------------------

Engine::Engine(DataBase *db) : Algorithm(), currentMovie(db)
{
   for (int f = 0; f < MAX_FEATURES; f++)
    {
        for (int i=0; i < MAX_MOVIES; i++)
            m_aMovieFeatures[f][i] = (float)INIT;
        for (int i=0; i < MAX_CUSTOMERS; i++)
            m_aCustFeatures[f][i] = (float)INIT;
    }
}

/**
 * Iteratively train each feature on the entire data set
 * Once sufficient progress has been made, move on
 */
void Engine::calcFeatures()
{
    double err, p, sq, rmse_last, rmse = 2.0;
    float cf, mf;

    for (int f = 0; f < 5 && f < MAX_FEATURES; f++)
    {
        printf("\n--- Calculating feature: %d ---\n", f);
        // Keep looping until you have passed a minimum number 
        // of epochs or have stopped making significant progress 
        User user(currentMovie.dataBase());
        int totalUsers = currentMovie.dataBase()->totalUsers();
        int movieId;
        for (int e = 0; (e < MIN_EPOCHS) || (rmse <= rmse_last - MIN_IMPROVEMENT); ++e)
        {
            sq = 0;
            rmse_last = rmse;
            int cacheId = 0;
            for (int custId = 0; custId < totalUsers; ++custId) {
                user.setId(custId);
                for (int v = 0; v < user.votes(); ++v)
                {
                    movieId = user.movie(v);
                    // Predict rating and calc error
                    p = predictRating(movieId, custId, f, cache[cacheId], true);
                    
                    err = (1.0 * user.score(v) - p);
                    sq += err * err;

                    // Cache off old feature values
                    cf = m_aCustFeatures[f][custId];
                    mf = m_aMovieFeatures[f][movieId];
                    // Cross-train the features
                    m_aCustFeatures[f][custId] += (float)(LRATE * (err * mf - K * cf));
                    m_aMovieFeatures[f][movieId] += (float)(LRATE * (err * cf - K * mf));
                }
                ++cacheId;
            }
            rmse = sqrt(sq / MAX_RATINGS);
            printf("     <set x='%d' y='%f' e='%d'/>\n", e * f, rmse, e);
        }

        int rank = 0;
        // Cache off old predictions
        for (int i = 0; i < currentMovie.dataBase()->totalUsers(); ++i) {
            user.setId(i);
            for (int v = 0; v < user.votes(); ++v)
            {
                cache[rank] = (float)predictRating(user.movie(i), i, f, cache[rank], false);
                ++rank;
            }
        }            
    }
}

/**
 * - During training there is no need to loop through all of the features
 * - Use a cache for the leading features and do a quick calculation for the trailing
 * - The trailing can be optionally removed when calculating a new cache value
 */
double Engine::predictRating(short movieId, int custId, int feature, float cache, bool bTrailing) const
{
    // Get cached value for old features or default to an average
    double sum = (cache > 0) ? cache : 1; //m_aMovies[movieId].PseudoAvg; 
    
    // Add contribution of current feature
    sum += m_aMovieFeatures[feature][movieId] * m_aCustFeatures[feature][custId];
    
    // Add up trailing defaults values
    if (bTrailing)
        sum += (MAX_FEATURES - feature - 1) * (INIT * INIT);
    qBound(1.0, sum, 5.0);
    return sum;
}

/**
 *  It loops through the entire list of finished features
 */
double Engine::determine(int user)
{
    double sum = 1; //m_aMovies[movieId].PseudoAvg;

    int movieId = currentMovie.id();
    for (int f=0; f < MAX_FEATURES; ++f) 
    {
        sum += m_aMovieFeatures[f][movieId] * m_aCustFeatures[f][user];
        qBound(1.0, sum, 5.0);
    }
    return sum;
}

int main(int argc, char **argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    
    DataBase db;
    db.load();
    Probe probe(&db);
    
    Engine *engine = new Engine(&db);
    engine->calcFeatures();
    int r = probe.runProbe(engine);
    delete engine;
    return r;
}
