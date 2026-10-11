#include <iostream>
#include <string>
#include <iomanip>
#include <vector>
#include <stack>
#include <fstream>

using namespace std;

const long long BASE = 257LL;
const long long MOD = 1000000007LL;

static LogNode* head = NULL;
static int TestCount = 1;

long long GetHighestPower(int k)
{
  long long power = 1;
  for (int i=0; i<k-1; i++)
    {
      power = (power*BASE)%MOD;
    }
  return power;
}

long long GetInitialHash(const string &text, int k)
{
  long long hashval = 0;
  for (int i=0; i<k; i++)
    {
      hashval = (hashval*BASE + (unsigned char)text[i])%MOD;
    }
  return hashval;
}

long long RollHash(long long oldhash, char oldc, char newc, long long power)
{
  long long outval = ((long long)(unsigned char)oldc * power)%MOD;
  long long rem = (oldhash-outval)%MOD;
  if (rem < 0) rem += MOD;
  long long newhash = (rem*BASE + (unsigned char)newc)%MOD;
  return newhash;
}
