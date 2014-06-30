#include "fwdmodel/thermal.hpp"
#include "fwdmodel/testtherm.hpp"
#include "app/console.hpp"

const int logLevel = -3;
const bool stdErr = false;
std::string directory = ".";

int main(int ac, char** av)
{
  obsidian::init::initialiseLogging("testthermal", logLevel, stdErr, directory);
  testing::InitGoogleTest(&ac, av);
  auto result = RUN_ALL_TESTS();
  return result;
}

