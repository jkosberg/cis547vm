/**
 * NOTE: You should feel free to manipulate any content in this .cpp file.
 * This means that if you want you can change almost everything,
 * as long as the fuzzer runs with the same cli interface.
 * This also means that if you're happy with some of the provided default
 * implementation, you don't have to modify it.
 */

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <numeric>
#include <unordered_set>

#include "Utils.h"

#define ARG_EXIST_CHECK(Name, Arg)            \
  {                                           \
    struct stat Buffer;                       \
    if (stat(Arg, &Buffer))                   \
    {                                         \
      fprintf(stderr, "%s not found\n", Arg); \
      return 1;                               \
    }                                         \
  }                                           \
  std::string Name(Arg);

#define DBG \
  std::cout << "Hit F::" << __FILE__ << " ::L" << __LINE__ << std::endl

/**
 * @brief Type Signature of Mutation Function.
 * MutationFn takes a string as input and returns a string.
 *
 * MutationFn: string -> string
 */
typedef std::string MutationFn(std::string);

/**
 * Struct that holds useful information about
 * one run of the program.
 *
 * @param Passed       did the program run without crashing?
 * @param Mutation     mutation function used for this run.
 * @param Input        parent input used for generating input for this run.
 * @param MutatedInput input string for this run.
 */
struct RunInfo
{
  bool Passed;
  MutationFn *Mutation;
  std::string Input, MutatedInput;
};

/************************************************/
/*            Global state variables            */
/************************************************/
/**
 * Note: Feel free to add/remove/change any of the following variables.
 * Depending on what you want to keep track of during fuzzing.
 */
// Collection of strings used to generate inputs
std::vector<std::string> SeedInputs;

// Variable to store coverage related information.
std::vector<std::string> CoverageState;

// Coverage related information from previous step.
std::vector<std::string> PrevCoverageState;

/**
 * @brief Variable to keep track of some Mutation related state.
 * Feel free to change/ignore this if you want to.
 */
int MutationState = 0;

/**
 * @brief Variable to keep track of some state related to strategy selection.
 * Feel free to change/ignore this if you want to.
 */
int StrategyState = 0;

/************************************************/
/*    Implement your select input algorithm     */
/************************************************/

// Global map to store the scores of each input
std::map<std::string, int> InputScores;

// Function to update the scores based on the feedback
void updateInputScores(RunInfo info, bool newCoverage)
{
  if (newCoverage || !info.Passed)
  {
    // encourage exploration of inputs that led to crash or new coverage
    InputScores[info.Input] += 10;
  }
}

/**
 * @brief Generate a completely random input string.
 *
 * @return std::string random input string.
 */
std::string generateRandomInput()
{
  int length = rand() % 256; // Random length between 0 and 255
  std::string randomInput;
  for (int i = 0; i < length; ++i)
  {
    randomInput += static_cast<char>(rand() % 256); // Random byte
  }
  return randomInput;
}

/**
 * @brief Select a string that will be mutated to generate a new input.
 * Sample code picks the first input string from SeedInputs.
 *
 * TODO: Implement your logic for selecting a input to mutate.
 * If you require, you can use the Info variable to help make a
 * decision while selecting a Seed but it is not necessary for the lab.
 *
 * @param RunInfo struct with information about the previous run.
 * @return Pointer to a string.
 */
std::string selectInput(RunInfo Info)
{
  const double randomSelectionProbability = 0.20;

  // Randomly explore new inputs
  if (rand() / ((double)RAND_MAX) < randomSelectionProbability)
  {
    // Fallback to a random seed input if nothing was selected yet
    int randomIndex = rand() % SeedInputs.size();
    return SeedInputs[randomIndex];
  }

  // prioritize re-exploring any input that has scored (indicating led to crash or new coverage)
  // choose random input from the scored inputs
  if (InputScores.size() > 0)
  {
    int randomIndex = rand() % InputScores.size();
    auto it = InputScores.begin();
    std::advance(it, randomIndex);
    return it->first;
  }
  // If no scored inputs, choose random input from the seed inputs
  int randomIndex = rand() % SeedInputs.size();
  return SeedInputs[randomIndex];
}

/*********************************************/
/*       Implement mutation strategies       */
/*********************************************/

const char ALPHA[] = "abcdefghijklmnopqrstuvwxyz\n\0";
const int LENGTH_ALPHA = sizeof(ALPHA);

/**
 * Here we provide a two sample mutation functions
 * that take as input a string and returns a string.
 */

/**
 * @brief Mutation Strategy that does nothing.
 *
 * @param Original Original input string.
 * @return std::string mutated string.
 */
std::string mutationA(std::string Original) { return Original; }

/**
 * @brief Mutation Strategy that inserts a random
 * alpha numeric char at a random location in Original.
 *
 * @param Original Original input string.
 * @return std::string mutated string.
 */
std::string mutationB(std::string Original)
{
  if (Original.length() <= 0)
    return Original;

  int Index = rand() % Original.length();
  return Original.insert(Index, 1, ALPHA[rand() % LENGTH_ALPHA]);
}

/**
 * TODO: Add your own mutation functions below.
 * Make sure to update the MutationFns vector to include your functions.
 *
 * Some ideas: swap adjacent chars, increment/decrement a char.
 *
 * Get creative with your strategies.
 */

/**
 * @brief Swap two adjacent bytes in the string at random
 *
 * @param original Original input string.
 * @return std::string mutated string.
 */
std::string swapAdjacentBytes(std::string original)
{
  if (original.length() <= 1)
    return original;

  int index = rand() % (original.length() - 1);
  std::swap(original[index], original[index + 1]);
  return original;
}

/**
 * @brief Increment a random byte in the string.
 *
 * @param original Original input string.
 * @return std::string mutated string.
 */
std::string incrementByte(std::string original)
{
  if (original.length() <= 0)
    return original;

  int index = rand() % original.length();
  original[index] = (original[index] + 1) % 256;
  return original;
}

/**
 * @brief Remove random byte from the string.
 *
 * @param original Original input string.
 * @return std::string mutated string.
 */
std::string removeByte(std::string original)
{
  if (original.length() <= 0)
    return original;

  int index = rand() % original.length();
  original.erase(index, 1);
  return original;
}

/**
 * @brief Add random byte to the string.
 *
 * @param original Original input string.
 * @return std::string mutated string.
 */

std::string addByte(std::string original)
{
  int index = rand() % (original.length() + 1);
  char newByte = rand() % 256;
  original.insert(index, 1, newByte);
  return original;
}

/**
 * @brief add random number of random bytes to the string.
 *
 * @param original Original input string.
 * @return std::string mutated string.
 */

std::string addRandomBytes(std::string original)
{
  // add random number of bytes from 1 to 256
  int numBytes = rand() % 256;
  // add random bytes
  for (int i = 0; i < numBytes; ++i)
  {
    char newByte = rand() % 256;
    original += newByte;
  }
  return original;
}

/**
 * @brief add random number of the same character to the string.
 *
 * @param original Original input string.
 * @return std::string mutated string.
 */
std::string addSameBytes(std::string original)
{
  // add random number of bytes from 1 to 256
  int numBytes = rand() % 256;

  // fix character
  char newByte = rand() % 256;
  // add random bytes
  for (int i = 0; i < numBytes; ++i)
  {

    original += newByte;
  }
  return original;
}

/**
 * Add specific length input
 * @param length Length of the input string.
 * @param character Character to fill the input string.
 * @return std::string input string.
 */
std::string generateSpecificLengthInput(int length, char character = 'a')
{
  std::string input;
  for (int i = 0; i < length; ++i)
  {
    input += character;
  }
  return input;
}

std::string addRandomNewlineBuffers(std::string original)
{
  // random length between 20 and 70
  auto lengthOne = rand() % 50 + 20;
  // length between 30 and 200
  auto lengthTwo = rand() % 170 + 30;
  // length between 120 and 300
  auto lengthThree = rand() % 180 + 120;

  std::string buf0 = generateSpecificLengthInput(lengthOne);   // length between 20 and 70
  std::string buf1 = generateSpecificLengthInput(lengthTwo);   // length between 30 and 200
  std::string buf2 = generateSpecificLengthInput(lengthThree); // length between 120 and 300

  return buf0 + "\n" + buf1 + "\n" + buf2 + "\n";
}

// Function to add random characters to the input string and set the 25th character
std::string addLengthAndSetCharacter(std::string original)
{
  // Ensure the input string is at least 25 characters long
  while (original.length() < 25)
  {
    original += static_cast<char>('a' + rand() % 26); // Add random lowercase letters
  }

  // Set the 25th character to 'a', 'b', or 'c'
  original[24] = "abc"[rand() % 3];

  // Add more random characters to increase the length
  int additionalLength = 250 + rand() % 100; // Random length between 250 and 350
  for (int i = 0; i < additionalLength; ++i)
  {
    original += static_cast<char>('a' + rand() % 26); // Add random lowercase letters
  }

  return original;
}

/**
 * @brief Reverse the entire input string.
 *
 * @param original Original input string.
 * @return std::string mutated string.
 */
std::string reverseString(std::string original)
{
  std::reverse(original.begin(), original.end());
  return original;
}

/**
 * @brief Duplicate a random byte in the string.
 *
 * @param original Original input string.
 * @return std::string mutated string.
 */
std::string duplicateRandomByte(std::string original)
{
  if (original.length() <= 0)
    return original;

  int index = rand() % original.length();
  char byteToDuplicate = original[index];
  original.insert(index, 1, byteToDuplicate);
  return original;
}

/**
 * @brief Duplicate a random substring within the string.
 *
 * @param original Original input string.
 * @return std::string mutated string.
 */
std::string duplicateSubstring(std::string original)
{
  if (original.length() < 2)
    return original;

  int start = rand() % original.length();
  int length = rand() % (original.length() - start);
  std::string substring = original.substr(start, length);
  int insertPos = rand() % original.length();
  original.insert(insertPos, substring);
  return original;
}

/**
 * @brief Flip a random bit in the string.
 *
 * @param original Original input string.
 * @return std::string mutated string.
 */
std::string flipRandomBit(std::string original)
{
  if (original.empty())
    return original;

  int byteIndex = rand() % original.length();
  int bitIndex = rand() % 8;
  original[byteIndex] ^= (1 << bitIndex);
  return original;
}

/**
 * @brief Vector containing all the available mutation functions
 *
 * TODO: Update the definition to include any mutations you implement.
 * For example if you implement mutationC then update it to be:
 * std::vector<MutationFn *> MutationFns = {mutationA, mutationB, mutationC};
 */
std::vector<MutationFn *> MutationFns = {
    mutationA,
    mutationB,
    swapAdjacentBytes,
    incrementByte,
    removeByte,
    addByte,
    addRandomBytes,
    addSameBytes,
    addRandomNewlineBuffers,
    addLengthAndSetCharacter,
    duplicateRandomByte,
    reverseString,
    duplicateSubstring,
    flipRandomBit};

// Global vector to store the scores of each mutation function
std::vector<int> MutationScores(MutationFns.size(), 1);

/**
 * @brief Update the mutation scores based on feedback of the previous run.
 * @param Info RunInfo struct with information about the previous run.
 * @param newCoverage boolean indicating if there was new line coverage
 */
void updateMutationScores(RunInfo &Info, bool newCoverage)
{
  // Find the function that was run
  int index = std::distance(MutationFns.begin(), std::find(MutationFns.begin(), MutationFns.end(), Info.Mutation));

  // If new coverage was found, increase the score significantly
  if (newCoverage)
  {
    MutationScores[index] += 10;
  }
  // increase score significantly if this mutation led to a crash
  if (!Info.Passed)
  {
    MutationScores[index] += 10;
  }
}

// Global vector to track the number of times each mutation function has been used
std::vector<int> MutationUsageCount(MutationFns.size(), 0);

// Minimum number of times each mutation function should be tried
const int MIN_TRIES_PER_MUTATION = 5;

/**
 * @brief Select a mutation function to apply to the seed input.
 * Sample code picks a random Strategy.
 *
 * TODO: Add your own logic to select a mutation function from MutationFns.
 * Hint: You may want to make use of any global state you store
 * during feedback to make decisions on what MutationFn to choose.
 *
 * @param RunInfo struct with information about the current run.
 * @returns a pointer to a MutationFn
 */
// Select a mutation function based on their scores with some randomness
MutationFn *selectMutationFn(RunInfo &Info)
{
  // Check if any mutation function has been used less than the minimum required times
  // This guarantees that all functions are used at least MIN_TRIES_PER_MUTATION times
  for (size_t i = 0; i < MutationUsageCount.size(); ++i)
  {
    if (MutationUsageCount[i] < MIN_TRIES_PER_MUTATION)
    {
      MutationUsageCount[i]++;
      return MutationFns[i];
    }
  }
  // Next, we use a weighted random selection based on the scores
  const double randomSelectionProbability = 0.20;

  // Select a random mutation function with a certain probability
  if ((rand() / (double)RAND_MAX) < randomSelectionProbability)
  {
    int randomIndex = rand() % MutationFns.size();
    return MutationFns[randomIndex];
  }

  // Select a mutation function based on their scores
  int totalScore = std::accumulate(MutationScores.begin(), MutationScores.end(), 0);

  if (totalScore == 0)
  {
    return MutationFns.front(); // Return a default mutation function if all scores are zero
  }

  // Generate a random number between 0 and totalScore - 1
  int randomScore = rand() % totalScore;
  int cumulativeScore = 0;

  // Iterate through the mutation functions and select one based on their scores
  for (size_t i = 0; i < MutationFns.size(); ++i)
  {
    cumulativeScore += MutationScores[i];
    if (randomScore < cumulativeScore)
    {
      return MutationFns[i];
    }
  }

  return MutationFns.back(); // Fallback in case of rounding errors
}
/*********************************************/
/*     Implement your feedback algorithm     */
/*********************************************/
/**
 * Update the internal state of the fuzzer using coverage feedback.
 *
 * @param Target name of target binary
 * @param Info RunInfo
 */
void feedBack(std::string &Target, RunInfo &Info)
{
  std::vector<std::string> RawCoverageData;
  readCoverageFile(Target, RawCoverageData);

  // track current and exisiting coverage lines in a set for easy lookup
  std::unordered_set<std::string> PrevCoverageSet(PrevCoverageState.begin(), PrevCoverageState.end());
  std::unordered_set<std::string> CurrentCoverageSet(CoverageState.begin(), CoverageState.end());

  // Check if there was any new coverage by seeing if the current coverage set contains something different
  // than the existing coverage set
  bool newCoverage = false;
  for (const auto &line : CurrentCoverageSet)
  {
    if (PrevCoverageSet.find(line) == PrevCoverageSet.end())
    {
      newCoverage = true;
      break;
    }
  }

  // Update the coverage state for the next run
  PrevCoverageState = CoverageState;
  // Clear the coverage state for the next run
  CoverageState.clear();
  // Update the mutation scores based on the feedback
  updateMutationScores(Info, newCoverage);
  updateInputScores(Info, newCoverage);

  // // dump current scores to file
  // std::string scorePath = Target + ".score";
  // std::ofstream scoreFile(scorePath, std::ios::trunc);
  // scoreFile << "Mutation Scores: ";
  // for (const auto &score : MutationScores)
  // {
  //   scoreFile << score << " ";
  // }
  // scoreFile << "\n";
  // scoreFile << "Input Scores: ";
  // for (const auto &pair : InputScores)
  // {
  //   scoreFile << pair.first << ": " << pair.second << " ";
  // }
  // scoreFile << "\n";
  // scoreFile.close();
  /**
   * TODO: Implement your logic to use the coverage information from the test
   * phase to guide fuzzing. The sky is the limit!
   *
   * Hint: You want to rely on some amount of randomness to make decisions.
   *
   * You have the Coverage information of the previous test in
   * PrevCoverageState. And the raw coverage data is loaded into RawCoverageData
   * from the Target.cov file. You can either use this raw data directly or
   * process it (not-necessary). If you do some processing, make sure to update
   * CoverageState to make it available in the next call to feedback.
   */
  CoverageState.assign(RawCoverageData.begin(),
                       RawCoverageData.end()); // No extra processing
}

int Freq = 1000;
int Count = 0;
int PassCount = 0;

bool test(std::string &Target, std::string &Input, std::string &OutDir)
{
  // Clean up old coverage file before running
  std::string CoveragePath = Target + ".cov";
  std::remove(CoveragePath.c_str());

  ++Count;
  int ReturnCode = runTarget(Target, Input);
  if (ReturnCode == 127)
  {
    fprintf(stderr, "%s not found\n", Target.c_str());
    exit(1);
  }
  fprintf(stderr, "\e[A\rTried %d inputs, %d crashes found\n", Count,
          failureCount);
  if (ReturnCode == 0)
  {
    if (PassCount++ % Freq == 0)
      storePassingInput(Input, OutDir);
    return true;
  }
  else
  {
    storeCrashingInput(Input, OutDir);
    return false;
  }
}

/**
 * @brief Fuzz the Target program and store the results to OutDir
 *
 * @param Target Target (instrumented) program binary.
 * @param OutDir Directory to store fuzzing results.
 */
void fuzz(std::string Target, std::string OutDir)
{
  struct RunInfo Info;
  while (true)
  {
    std::string Input = selectInput(Info);
    Info = RunInfo();
    Info.Input = Input;
    Info.Mutation = selectMutationFn(Info);
    Info.MutatedInput = Info.Mutation(Info.Input);
    Info.Passed = test(Target, Info.MutatedInput, OutDir);
    feedBack(Target, Info);
  }
}

/**
 * Usage:
 * ./fuzzer [target] [seed input dir] [output dir] [frequency] [random seed]
 */
int main(int argc, char **argv)
{
  if (argc < 4)
  {
    printf("usage %s [target] [seed input dir] [output dir] [frequency "
           "(optional)] [seed (optional arg)]\n",
           argv[0]);
    return 1;
  }

  ARG_EXIST_CHECK(Target, argv[1]);
  ARG_EXIST_CHECK(SeedInputDir, argv[2]);
  ARG_EXIST_CHECK(OutDir, argv[3]);

  if (argc >= 5)
    Freq = strtol(argv[4], NULL, 10);

  int RandomSeed = argc > 5 ? strtol(argv[5], NULL, 10) : (int)time(NULL);

  srand(RandomSeed);
  storeSeed(OutDir, RandomSeed);
  initialize(OutDir);

  if (readSeedInputs(SeedInputs, SeedInputDir))
  {
    fprintf(stderr, "Cannot read seed input directory\n");
    return 1;
  }
  fprintf(stderr, "Fuzzing %s...\n\n", Target.c_str());
  fuzz(Target, OutDir);

  // At the end, dump the scores for all fuzzing functions to a file
  // Ensure the "failure" directory exists within OutDir
  std::string failureDir = OutDir + "/failure";
  return 0;
}
