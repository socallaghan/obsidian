//!
//! Input implementations related to magnetics forward model.
//!
//! \file magnetism.cpp
//! \author Lachlan McCalman
//! \author Nahid Akbar
//! \date 2014
//! \license General Public License version 3 or later
//! \copyright (c) 2014, NICTA
//!

#include "common.hpp"

namespace obsidian
{
  template<>
  std::string configHeading<ForwardModel::MAGNETICS>()
  {
    return "magnetism";
  }

  template<>
  void initSensorInputFileOptions<ForwardModel::MAGNETICS>(po::options_description & options)
  {
    options.add_options() //
    ("magnetism.enabled", po::value<bool>(), "enable sensor") //
    ("magnetism.sensorLocations", po::value<std::string>(), "sensor locations") //
    ("magnetism.sensorReadings", po::value<std::string>(), "sensor readings") //
    ("magnetism.gridResolution", po::value<Eigen::Vector3i>(), "grid points per cube side") //
    ("magnetism.noiseAlpha", po::value<double>(), "noise NIG alpha variable") //
    ("magnetism.noiseBeta", po::value<double>(), "noise NIG beta variable") //
    ("magnetism.supersample", po::value<uint>(), "supersampling exponent") //
    ("magnetism.magneticField", po::value<Eigen::Vector3d>(), "magnetic field of location");
  }

  template<>
  MagSpec parseSpec(const po::variables_map& vm, const std::set<ForwardModel> & sensorsEnabled)
  {
    MagSpec spec;
    if (sensorsEnabled.count(ForwardModel::MAGNETICS))
    {
      spec.locations = io::csv::read<double>(vm["magnetism.sensorLocations"].as<std::string>());
      Eigen::Vector3i magRes = vm["magnetism.gridResolution"].as<Eigen::Vector3i>();
      spec.voxelisation.xResolution = uint(magRes(0));
      spec.voxelisation.yResolution = uint(magRes(1));
      spec.voxelisation.zResolution = uint(magRes(2));
      spec.voxelisation.supersample = vm["magnetism.supersample"].as<uint>();
      spec.noise.inverseGammaAlpha = vm["magnetism.noiseAlpha"].as<double>();
      spec.noise.inverseGammaBeta = vm["magnetism.noiseBeta"].as<double>();
      Eigen::Vector3d magFld = vm["magnetism.magneticField"].as<Eigen::Vector3d>();
      spec.backgroundField = magFld;
    }
    return spec;
  }

  template<>
  po::variables_map write<>(const std::string & prefix, MagSpec spec, const po::options_description & od)
  {
    io::csv::write(prefix + "sensorLocations.csv", spec.locations);

    return build_vm(
        po::variables_map(),
        od,
        "magnetism",
        { { "sensorLocations", prefix + "sensorLocations.csv" },
          { "gridResolution", io::to_string(spec.voxelisation.xResolution, spec.voxelisation.yResolution, spec.voxelisation.zResolution) },
          { "supersample", io::to_string(spec.voxelisation.supersample) },
          { "noiseAlpha", io::to_string(spec.noise.inverseGammaAlpha) },
          { "noiseBeta", io::to_string(spec.noise.inverseGammaBeta) },
          { "magneticField", io::to_string(spec.backgroundField[0], spec.backgroundField[1], spec.backgroundField[2]) } });
  }

  //! @note the sensor params don't actually have anything in them at the moment so we don't need to do any parsing
  //!
  template<>
  MagParams parseSimulationParams(const po::variables_map& vm, const std::set<ForwardModel> & sensorsEnabled)
  {
    return MagParams();
  }
  template<>
  MagResults parseSensorReadings(const po::variables_map& vm, const std::set<ForwardModel> & sensorsEnabled)
  {

    MagResults s;
    if (sensorsEnabled.count(ForwardModel::MAGNETICS))
    {
      s.readings = io::csv::read<double, Eigen::Dynamic, 1>(vm["magnetism.sensorReadings"].as<std::string>());
      s.likelihood = 0.0;
    }
    return s;
  }

  template<>
  po::variables_map write<>(const std::string & prefix, MagResults g, const po::options_description & od)
  {
    io::csv::write<double, Eigen::Dynamic, 1>(prefix + "sensorReadings.csv", g.readings);
    return build_vm(po::variables_map(), od, "magnetism", { { "sensorReadings", prefix + "sensorReadings.csv" } });
  }

  template<>
  void enableProperties<ForwardModel::MAGNETICS>(Eigen::VectorXi & propMasksMask)
  {
    propMasksMask(static_cast<uint>(RockProperty::LogSusceptibility)) = 1;
  }

  template<>
  prior::MagParamsPrior parsePrior(const po::variables_map& vm, const std::set<ForwardModel> & sensorsEnabled)
  {
    return prior::MagParamsPrior();
  }

  template<>
  bool validateSensor(const WorldSpec & world, const MagSpec & spec, const MagResults & result)
  {
    bool valid = true;
    if (spec.locations.rows() == 0)
    {
      LOG(ERROR)<< "input: no magnetism locations specified. Disable forward model if it is not used.";
      valid = false;
    }
    if (static_cast<uint>(spec.locations.cols()) != 3)
    {
      LOG(ERROR)<< "input: locations in magnetism must have three cols (x, y, z).";
      valid = false;
    }
    for (uint l = 0; l < spec.locations.rows(); l++)
    {
      if (spec.locations(l, 0) < world.xBounds.first || spec.locations(l, 0) > world.xBounds.second
          || spec.locations(l, 1) < world.yBounds.first || spec.locations(l, 1) > world.yBounds.second)
      {
        LOG(ERROR)<< "input: magnetism location " << (l + 1) << " is out of world bounds";
        valid = false;
      }
    }
    if (spec.voxelisation.xResolution <= 0 || spec.voxelisation.yResolution <= 0 || spec.voxelisation.zResolution <= 0)
    {
      LOG(ERROR)<< "input: magnetism voxelisation (x, y, z)must be greater than 0.";
      valid = false;
    }
    if (spec.noise.inverseGammaAlpha <= 0 || spec.noise.inverseGammaAlpha <= 0)
    {
      LOG(ERROR)<< "input: magnetism noise parameters must be greater than 0";
      valid = false;
    }
    if (static_cast<uint>(spec.locations.rows()) != result.readings.size())
    {
      LOG(ERROR)<< "input: different number of readings for magnetism results (" << result.readings.size() << ") to location specified. Remove or add more locations ("<< spec.locations.rows() <<").";
      valid = false;
    }
    return valid;
  }
}
