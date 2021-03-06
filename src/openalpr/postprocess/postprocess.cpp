/*
 * Copyright (c) 2015 New Designs Unlimited, LLC
 * Opensource Automated License Plate Recognition [http://www.openalpr.com]
 *
 * This file is part of OpenAlpr.
 *
 * OpenAlpr is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License
 * version 3 as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "postprocess.h"

using namespace std;

namespace alpr
{

  PostProcess::PostProcess(Config* config)
  {
    this->config = config;

    stringstream filename;
    filename << config->getPostProcessRuntimeDir() << "/" << config->country << ".patterns";

    std::ifstream infile(filename.str().c_str());

    string region, pattern;
    while (infile >> region >> pattern)
    {
      RegexRule* rule = new RegexRule(region, pattern);
      //cout << "REGION: " << region << " PATTERN: " << pattern << endl;

      if (rules.find(region) == rules.end())
      {
        vector<RegexRule*> newRule;
        newRule.push_back(rule);
        rules[region] = newRule;
      }
      else
      {
        vector<RegexRule*> oldRule = rules[region];
        oldRule.push_back(rule);
        rules[region] = oldRule;
      }
    }

    //vector<RegexRule> test = rules["base"];
    //for (int i = 0; i < test.size(); i++)
    //  cout << "Rule: " << test[i].regex << endl;
  }

  PostProcess::~PostProcess()
  {
    // TODO: Delete all entries in rules vector
    map<string, vector<RegexRule*> >::iterator iter;

    for (iter = rules.begin(); iter != rules.end(); ++iter)
    {
      for (int i = 0; i < iter->second.size(); i++)
      {
        delete iter->second[i];
      }
    }
  }

  void PostProcess::addLetter(string letter, int charposition, float score)
  {
    if (score < config->postProcessMinConfidence)
      return;

    insertLetter(letter, charposition, score);

    if (score < config->postProcessConfidenceSkipLevel)
    {
      float adjustedScore = abs(config->postProcessConfidenceSkipLevel - score) + config->postProcessMinConfidence;
      insertLetter(SKIP_CHAR, charposition, adjustedScore );
    }

    //if (letter == '0')
    //{
    //  insertLetter('O', charposition, score - 0.5);
    //}
  }

  void PostProcess::insertLetter(string letter, int charposition, float score)
  {
    score = score - config->postProcessMinConfidence;

    int existingIndex = -1;
    if (letters.size() < charposition + 1)
    {
      for (int i = letters.size(); i < charposition + 1; i++)
      {
        vector<Letter> tmp;
        letters.push_back(tmp);
      }
    }

    for (int i = 0; i < letters[charposition].size(); i++)
    {
      if (letters[charposition][i].letter == letter &&
          letters[charposition][i].charposition == charposition)
      {
        existingIndex = i;
        break;
      }
    }

    if (existingIndex == -1)
    {
      Letter newLetter;
      newLetter.charposition = charposition;
      newLetter.letter = letter;
      newLetter.occurences = 1;
      newLetter.totalscore = score;
      letters[charposition].push_back(newLetter);
    }
    else
    {
      letters[charposition][existingIndex].occurences = letters[charposition][existingIndex].occurences + 1;
      letters[charposition][existingIndex].totalscore = letters[charposition][existingIndex].totalscore + score;
    }
  }

  void PostProcess::clear()
  {
    for (int i = 0; i < letters.size(); i++)
    {
      letters[i].clear();
    }
    letters.resize(0);

    unknownCharPositions.clear();
    unknownCharPositions.resize(0);
    allPossibilities.clear();
    allPossibilitiesLetters.clear();
    //allPossibilities.resize(0);

    bestChars = "";
    matchesTemplate = false;
  }

  void PostProcess::analyze(string templateregion, int topn)
  {
    timespec startTime;
    getTimeMonotonic(&startTime);

    // Get a list of missing positions
    for (int i = letters.size() -1; i >= 0; i--)
    {
      if (letters[i].size() == 0)
      {
        unknownCharPositions.push_back(i);
      }
    }

    if (letters.size() == 0)
      return;

    // Sort the letters as they are
    for (int i = 0; i < letters.size(); i++)
    {
      if (letters[i].size() > 0)
        sort(letters[i].begin(), letters[i].end(), letterCompare);
    }

    if (this->config->debugPostProcess)
    {
      // Print all letters
      for (int i = 0; i < letters.size(); i++)
      {
        for (int j = 0; j < letters[i].size(); j++)
          cout << "PostProcess Letter: " << letters[i][j].charposition << " " << letters[i][j].letter << " -- score: " << letters[i][j].totalscore << " -- occurences: " << letters[i][j].occurences << endl;
      }
    }

    timespec permutationStartTime;
    getTimeMonotonic(&permutationStartTime);

    findAllPermutations(templateregion, topn);

    if (config->debugTiming)
    {
      timespec permutationEndTime;
      getTimeMonotonic(&permutationEndTime);
      cout << " -- PostProcess Permutation Time: " << diffclock(permutationStartTime, permutationEndTime) << "ms." << endl;
    }

    if (allPossibilities.size() > 0)
    {

      bestChars = allPossibilities[0].letters;
      for (int z = 0; z < allPossibilities.size(); z++)
      {
        if (allPossibilities[z].matchesTemplate)
        {
          bestChars = allPossibilities[z].letters;
          break;
        }
      }

      // Now adjust the confidence scores to a percentage value
      float maxPercentScore = calculateMaxConfidenceScore();
      float highestRelativeScore = (float) allPossibilities[0].totalscore;

      for (int i = 0; i < allPossibilities.size(); i++)
      {
        allPossibilities[i].totalscore = maxPercentScore * (allPossibilities[i].totalscore / highestRelativeScore);
      }
    }

    if (this->config->debugPostProcess)
    {
      // Print top words
      for (int i = 0; i < allPossibilities.size(); i++)
      {
        cout << "Top " << topn << " Possibilities: " << allPossibilities[i].letters << " :\t" << allPossibilities[i].totalscore;
        if (allPossibilities[i].letters == bestChars)
          cout << " <--- ";
        cout << endl;
      }
      cout << allPossibilities.size() << " total permutations" << endl;
    }

    if (config->debugTiming)
    {
      timespec endTime;
      getTimeMonotonic(&endTime);
      cout << "PostProcess Time: " << diffclock(startTime, endTime) << "ms." << endl;
    }

    if (this->config->debugPostProcess)
      cout << "PostProcess Analysis Complete: " << bestChars << " -- MATCH: " << matchesTemplate << endl;
  }

  bool PostProcess::regionIsValid(std::string templateregion)
  {
    return rules.find(templateregion) != rules.end();
  }
  
  float PostProcess::calculateMaxConfidenceScore()
  {
    // Take the best score for each char position and average it.

    float totalScore = 0;
    int numScores = 0;
    // Get a list of missing positions
    for (int i = 0; i < letters.size(); i++)
    {
      if (letters[i].size() > 0)
      {
        totalScore += (letters[i][0].totalscore / letters[i][0].occurences) + config->postProcessMinConfidence;
        numScores++;
      }
    }

    if (numScores == 0)
      return 0;

    return totalScore / ((float) numScores);
  }

  const vector<PPResult> PostProcess::getResults()
  {
    return this->allPossibilities;
  }

  struct PermutationCompare {
    bool operator() (pair<float,vector<int> > &a, pair<float,vector<int> > &b)
    {
      return (a.first < b.first);
    }
  };

  void PostProcess::findAllPermutations(string templateregion, int topn) {

    // use a priority queue to process permutations in highest scoring order
    priority_queue<pair<float,vector<int> >, vector<pair<float,vector<int> > >, PermutationCompare> permutations;
    set<float> permutationHashes;

    // push the first word onto the queue
    float totalscore = 0;
    for (int i=0; i<letters.size(); i++)
    {
      if (letters[i].size() > 0)
        totalscore += letters[i][0].totalscore;
    }
    vector<int> v(letters.size());
    permutations.push(make_pair(totalscore, v));

    int consecutiveNonMatches = 0;
    while (permutations.size() > 0)
    {
      // get the top permutation and analyze
      pair<float, vector<int> > topPermutation = permutations.top();
      if (analyzePermutation(topPermutation.second, templateregion, topn) == true)
        consecutiveNonMatches = 0;
      else
        consecutiveNonMatches += 1;
      permutations.pop();

      if (allPossibilities.size() >= topn || consecutiveNonMatches >= 10)
        break;

      // add child permutations to queue
      for (int i=0; i<letters.size(); i++)
      {
        // no more permutations with this letter
        if (topPermutation.second[i]+1 >= letters[i].size())
          continue;

        pair<float, vector<int> > childPermutation = topPermutation;
        childPermutation.first -= letters[i][topPermutation.second[i]].totalscore - letters[i][topPermutation.second[i] + 1].totalscore;
        childPermutation.second[i] += 1;

        // ignore permutations that have already been visited (assume that score is a good hash for permutation)
        if (permutationHashes.end() != permutationHashes.find(childPermutation.first))
          continue;

        permutations.push(childPermutation);
        permutationHashes.insert(childPermutation.first);
      }
    }
  }

  bool PostProcess::analyzePermutation(vector<int> letterIndices, string templateregion, int topn)
  {
    PPResult possibility;
    possibility.letters = "";
    possibility.totalscore = 0;
    possibility.matchesTemplate = false;
    int plate_char_length = 0;

    for (int i = 0; i < letters.size(); i++)
    {
      if (letters[i].size() == 0)
        continue;

      Letter letter = letters[i][letterIndices[i]];

      if (letter.letter != SKIP_CHAR)
      {
        possibility.letters = possibility.letters + letter.letter;
        possibility.letter_details.push_back(letter);
        plate_char_length += 1;
      }
      possibility.totalscore = possibility.totalscore + letter.totalscore;
    }

    // ignore plates that don't fit the length requirements
    if (plate_char_length < config->postProcessMinCharacters ||
      plate_char_length > config->postProcessMaxCharacters)
      return false;

    // Apply templates
    if (templateregion != "")
    {
      vector<RegexRule*> regionRules = rules[templateregion];

      for (int i = 0; i < regionRules.size(); i++)
      {
        possibility.matchesTemplate = regionRules[i]->match(possibility.letters);
        if (possibility.matchesTemplate)
        {
          possibility.letters = regionRules[i]->filterSkips(possibility.letters);
          break;
        }
      }
    }

    // ignore duplicate words
    if (allPossibilitiesLetters.end() != allPossibilitiesLetters.find(possibility.letters))
      return false;

    allPossibilities.push_back(possibility);
    allPossibilitiesLetters.insert(possibility.letters);
    return true;
  }

  bool wordCompare( const PPResult &left, const PPResult &right )
  {
    if (left.totalscore < right.totalscore)
      return false;
    return true;
  }

  bool letterCompare( const Letter &left, const Letter &right )
  {
    if (left.totalscore < right.totalscore)
      return false;
    return true;
  }

}