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
#include <qbitarray.h>

/**
    One of the basic algorithm is the average algorithm which takes
    the average of all the votes for a movie and guess's that for any user.

*/
/*
class DoubleAverage : public Algorithm {
public:
    DoubleAverage(DataBase *db) : currentMovie(db), user(db), otheruser(db), currentMovieAverage(-1) {}

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
        for (int i=0;i<6;++i)
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
                bestScore = votedSame*seenSame;
                bestUser = i;
            }
        }
        int r = currentMovie.findScore(currentMovie.user(bestUser));
        int best = 0;
        for (int i=1;i<6;++i) {
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
*/

class Group {
public:
    int score;
    QVector<int> users;
    QVector<int> movies;
};

#include <math.h>

void findGroups(const QVector<int>users, int rootMovie, DataBase *db, int score)
{
    Movie currentMovie(db);
    // generate all posible combinations for users
    
    int totalMovies = db->totalMovies();

    int minimum = 6;
    int none = 0;
    QVector<int> mtc;
    QVector<int> used;
    used.resize(users.count());
    for (int j = 1; j < totalMovies + 1; ++j) {
        if (j == rootMovie) {
            none++;
            continue;
        }
        currentMovie.setId(j);
        int c = 0;
        for (int k = 0; k < users.count(); ++k) {
            int r = currentMovie.findVote(users.at(k));
            if (r == -1 || currentMovie.score(r) != score) {
            } else {
                used[k] = 1;
                c++;
            }
        }
        if (c <= minimum) {
            none++;
        } else {
            mtc.append(j);
        }
    }
    qDebug() << "none" << none << mtc.count() << users.count() << score;
    int c = 0;
    for (int i = 0; i < used.count(); ++i) {
        if (used.at(i) == 1)
            c++;
    }
    qDebug() << "used" << c << used.count();

    QVector<QVector<int> > cache;
    for (int i = 0; i < mtc.count(); ++i) {
        currentMovie.setId(mtc.at(i));
        cache.append(QVector<int>());
        for (int j = 0; j < users.count(); ++j) {
            int r = currentMovie.findVote(users.at(j));
            if (r != -1 && currentMovie.score(r) != score)
                r = -1;
            cache[i].append(r != -1);
        }
    }
    for (int i = 0; i < cache.count(); ++i) {
        QString line;
        for ( int j = 0; j < cache.at(i).count(); ++j) {
            line += QString("%1").arg(cache.at(i).at(j));
        }
        //qDebug() << line;
    }
    
    /*
    QVector<QBitArray> cache;
    for (int i = 0; i < mtc.count(); ++i) {
        currentMovie.setId(mtc.at(i));
        cache.append(QBitArray(users.count()));
        for (int j = 0; j < users.count(); ++j) {
            int r = currentMovie.findVote(users.at(j));
            if (r != -1 && currentMovie.score(r) != score)
                r = -1;
           cache[i].setBit(j, (r == -1));
        }
    }
    */

    // this only works up to 32 users for now
    int total = (int)(exp2(users.count()));
    qDebug() << total << users.count() << mtc.count();
    QBitArray combinations(users.count());
    while (combinations.count(true) < users.count()) {
        //if (combinations.count(true) > 15) {
        //    QString x;
        //    for (int i=0; i<combinations.count(); ++i)
        //        x += combinations.testBit(i) ? "1" : "0";
        //    qDebug() << x << combinations.count(true) << users.count();
        //}
        QVector<int> currentTestGroup;
        for (int j = 0; j < users.count(); ++j) {
            if (combinations.testBit(j))
                currentTestGroup.append(j);
        }
        if (currentTestGroup.count() > minimum) {
 /*
            qDebug() << currentTestGroup;
        }
        int carry = 1;
        int offset = 0;
        while (carry) {
            carry = (combinations.toggleBit(offset++));
        }
        
    }
    for (int i = 1; i < total; ++i) {
        QVector<int> currentTestGroup;
        for (int j = 0; j < users.count(); ++j) {
            if ((i & (1 << j)) != 0)
                currentTestGroup.append(j);
        }
        if (currentTestGroup.count() <= minimum)
            continue;
   */
            Group g;
        int mtcCount = mtc.count();
        int currentTestGroupCount = currentTestGroup.count();
        for (int j = 0; j < mtcCount; ++j) {
            for (int k = 0; k < currentTestGroupCount; ++k) {
                if (0 == cache.at(j).at(currentTestGroup.at(k)))
                //if (cache.at(j).testBit(currentTestGroup.at(k)))
                    break;
                if ((k + 1) == currentTestGroupCount) {
                    g.movies.append(mtc.at(j));
                }
            }
        }
        if (g.movies.count() > 2) {
            g.score = score;
            g.users = currentTestGroup;
            g.movies.append(rootMovie);
            QList<int> l;
            for (int j=0;j <g.users.count();j++) {
                l.append(users.at(g.users.at(j)));
            }
            qDebug() << "users" << l << "all scored" << g.score << "on movies:" << g.movies.count() << g.movies;
        }
        //if (i % (total/10000) == 0)
        //    qDebug() << i/(total/100) << "%" << total << i;
        //if (i % (total/1000) == 0)
        //    return;
        }
        
        
        int carry = 1;
        int offset = 0;
        while (carry == 1) {
            carry = (combinations.toggleBit(offset++));
        }
    }
}

void findMatches(DataBase *db)
{   
    //for (int i = 0; i < db->totalMovies(); ++i) {
        Movie currentMovie(db, 1);
        for (int score = 1; score < 2; ++score) {
            QVector<int> users;
            for (uint j = 0; j < currentMovie.votes(); ++j) {
                int user = currentMovie.user(j);
                if (currentMovie.score(j) == score)
                    users.append(user);
            }
            findGroups(users, currentMovie.id(), db, score);
        }
    //}
}

int main(int , char **)
{
    DataBase db;
    db.load();
    findMatches(&db);
    return 0;
//    Probe probe(&db);
//    DoubleAverage bf(&db);
//    return probe.runProbe(&bf);
}

