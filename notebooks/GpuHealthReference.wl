(* GPU Health Algorithm - Wolfram Language source
   This is the plain Wolfram Language calculation source used by the .wls runner. *)

ClearAll[clamp, mean, variance, medianAbsoluteDeviation, robustZScore,
  exponentialSmooth, pearsonCorrelation, regressionSlope, analyzeGpuTelemetry];

clamp[x_, lo_, hi_] := Min[hi, Max[lo, x]];
mean[x_List] := If[Length[x] == 0, 0.0, N[Mean[x]]];
variance[x_List] := If[Length[x] < 2, 0.0, N[Variance[x]]];
medianAbsoluteDeviation[x_List] := Module[{m = Median[x]}, Median[Abs[x - m]]];
robustZScore[x_, m_, mad_] := If[mad <= 10^-9, 0.0, 0.6745*(x - m)/mad];

exponentialSmooth[values_List, alpha_:0.35] := Module[{out},
  If[Length[values] == 0, Return[{}]];
  out = {First[values]};
  Do[AppendTo[out, alpha*values[[i]] + (1 - alpha)*Last[out]], {i, 2, Length[values]}];
  out
];

pearsonCorrelation[x_List, y_List] := Module[{},
  If[Length[x] != Length[y] || Length[x] < 2, Return[0.0]];
  Quiet@Check[N[Correlation[x, y]], 0.0]
];

regressionSlope[x_List, y_List] := Module[{mx, my, num, den},
  If[Length[x] != Length[y] || Length[x] < 2, Return[0.0]];
  mx = Mean[x]; my = Mean[y];
  num = Total[(x - mx)*(y - my)];
  den = Total[(x - mx)^2];
  If[den <= 10^-9, 0.0, N[num/den]]
];

analyzeGpuTelemetry[samples_List] := Module[
  {sorted, t0, t, temp, power, limit, util, mem, throttle, smoothTemp,
   smoothPower, smoothUtil, smoothMem, tempNow, slope, predictedTemp,
   powerMean, cvPower, powerDiff, ratios, powerRatio, powerInstability,
   utilVarianceRisk, utilPowerCorr, utilTempCorr, utilizationStability,
   memDiff, memoryPressure, tempMedian, tempMad, powerMedian, powerMad,
   utilMedian, utilMad, anomalyCount, anomalyRisk, thermalRisk, throttleRisk,
   healthScore, diagnosis},

  sorted = SortBy[samples, #timestampUnixMs&];
  t0 = sorted[[1, "timestampUnixMs"]];
  t = (#["timestampUnixMs"] - t0)/1000.0& /@ sorted;
  temp = Lookup[#, "temperatureCelsius", 55.0]& /@ sorted;
  power = Lookup[#, "powerWatts", 80.0]& /@ sorted;
  limit = Max[Lookup[#, "powerLimitWatts", 140.0], 1.0]& /@ sorted;
  util = Lookup[#, "gpuUtilizationPercent", 40.0]& /@ sorted;
  mem = clamp[Lookup[#, "memoryUtilizationPercent", 35.0], 0.0, 100.0]& /@ sorted;
  throttle = If[Lookup[#, "throttleReasons", 0] != 0, 1.0, 0.0]& /@ sorted;

  smoothTemp = exponentialSmooth[temp, 0.35];
  smoothPower = exponentialSmooth[power, 0.35];
  smoothUtil = exponentialSmooth[util, 0.35];
  smoothMem = exponentialSmooth[mem, 0.35];

  tempNow = Last[smoothTemp];
  slope = regressionSlope[t, smoothTemp];
  predictedTemp = tempNow + 60.0*slope;
  powerMean = Max[mean[smoothPower], 1.0];
  cvPower = StandardDeviation[smoothPower]/powerMean;
  powerDiff = Abs[Differences[smoothPower]]/powerMean;
  ratios = power/limit;
  powerRatio = clamp[mean[ratios], 0.0, 1.5];
  powerInstability = clamp[0.45*cvPower + 0.35*mean[powerDiff] + 0.20*clamp[(powerRatio - 0.75)/0.25, 0.0, 1.0], 0.0, 1.0];

  utilVarianceRisk = clamp[variance[smoothUtil]/900.0, 0.0, 1.0];
  utilPowerCorr = Abs[pearsonCorrelation[smoothUtil, smoothPower]];
  utilTempCorr = Abs[pearsonCorrelation[smoothUtil, smoothTemp]];
  utilizationStability = clamp[1.0 - 0.55*utilVarianceRisk + 0.30*utilPowerCorr + 0.15*(1.0 - utilTempCorr), 0.0, 1.0];

  memDiff = Abs[Differences[smoothMem]]/100.0;
  memoryPressure = clamp[0.70*(mean[smoothMem]/100.0) + 0.30*mean[memDiff], 0.0, 1.0];

  tempMedian = Median[temp]; tempMad = medianAbsoluteDeviation[temp];
  powerMedian = Median[power]; powerMad = medianAbsoluteDeviation[power];
  utilMedian = Median[util]; utilMad = medianAbsoluteDeviation[util];
  anomalyCount = Count[Range[Length[sorted]], i_ /; Abs[robustZScore[temp[[i]], tempMedian, tempMad]] > 3.5 || Abs[robustZScore[power[[i]], powerMedian, powerMad]] > 3.5 || Abs[robustZScore[util[[i]], utilMedian, utilMad]] > 3.5];
  anomalyRisk = clamp[N[anomalyCount/Length[sorted]], 0.0, 1.0];
  thermalRisk = clamp[0.55*((tempNow - 70.0)/18.0) + 0.30*((predictedTemp - 78.0)/17.0) + 0.15*clamp[slope/0.25, 0.0, 1.0], 0.0, 1.0];
  throttleRisk = clamp[0.45*mean[throttle] + 0.30*thermalRisk + 0.25*clamp[(powerRatio - 0.90)/0.10, 0.0, 1.0], 0.0, 1.0];
  healthScore = clamp[100.0*(1.0 - 0.30*thermalRisk - 0.20*powerInstability - 0.15*memoryPressure - 0.15*anomalyRisk - 0.20*throttleRisk), 0.0, 100.0];
  diagnosis = If[healthScore >= 90.0, "Excellent GPU condition", If[healthScore >= 75.0, "GPU is stable", "GPU requires attention"]] <> ".";

  <|"healthScore" -> healthScore, "thermalRisk" -> thermalRisk,
    "powerInstability" -> powerInstability, "memoryPressure" -> memoryPressure,
    "utilizationStability" -> utilizationStability,
    "predictedTemperature60s" -> predictedTemp, "anomalyCount" -> anomalyCount,
    "diagnosis" -> diagnosis|>
];
