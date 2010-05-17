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

#include <probe.h>
#include <user.h>

/*
 
*/
class DoubleAverage : public Algorithm
{
public:
    DoubleAverage(DataBase *db) : currentMovie(db), user(db), otheruser(db), currentMovieAverage(-1)
    {}

    void setMovie(int id)
    {
        currentMovie.setId(id);
        currentMovieAverage = -1;
    }

    double determine(int userId)
    {
        QList<int> users;
        int bestScore = 0;
        int bestUser = 0;
        user.setId(userId);
        QHash<int, int> scores;
        for (uint j = 0; j < user.votes(); ++j)
            scores[user.movie(j)] = user.score(j);

        int bestScores[6];
        for (int i = 0;i < 6;++i)
            bestScores[i] = 0;
        for (uint i = 0; i < currentMovie.votes(); ++i) {
            int currentUser = currentMovie.user(i);
            if (currentUser == userId)
                continue;
            otheruser.setId(currentUser);
            int seenSame = 0;
            int votedSame = 0;

            for (uint j = 0; j < otheruser.votes(); ++j) {
                int movie = otheruser.movie(j);
                if (scores.contains(movie)) {
                    int otherScore = otheruser.score(j);
                    if (otherScore == scores[movie]) {
                        votedSame++;
                    }
                    seenSame++;
                }
            }
            if (votedSame > 2 && seenSame > 4) {
                bestScores[currentMovie.score(i)]++;
            }
            if (votedSame * seenSame > bestScore) {
                bestScore = votedSame * seenSame;
                bestUser = i;
            }
        }
        int r = currentMovie.findScore(currentMovie.user(bestUser));
        int best = 0;
        for (int i = 1;i < 6;++i) {
            if (bestScores[i] > bestScores[best]) {
                best = i;
            }
        }
        qDebug() << "real score:" << currentMovie.findScore(userId) << "guess" << r << best;
        if (best != 0) {
            return best;
        }
        return r;
    }

    Movie currentMovie;
    User user;
    User otheruser;
    double currentMovieAverage;
};

int main(int , char **)
{
    DataBase db;
    db.load();
    return 0;
    Probe probe(&db);
    DoubleAverage bf(&db);
    return probe.runProbe(&bf);
}

