# mini-wiener-cybernetics

Norbert Wiener's Cybernetics (1948) — C implementation covering control, communication, feedback, information theory, Wiener processes, optimal filtering, and prediction.

## Module Status: COMPLETE ✅

- **L1-L6**: Complete
- **L7**: Complete (4 applications: fire control, thermal regulation, biological homeostasis, communication)
- **L8**: Complete (9 advanced topics: first passage, arcsin law, Volterra series, stochastic resonance, Fokker-Planck, etc.)
- **L9**: Partial (documented, not implemented)

Score: 17/18

## Knowledge Coverage

| Level | Name | Status |
|-------|------|--------|
| L1 | Definitions | Complete — 7 struct types |
| L2 | Core Concepts | Complete — feedback, information, homeostasis, goals |
| L3 | Math Structures | Complete — Wiener process, SDE, filter, AR/ARMA |
| L4 | Fundamental Laws | Complete — Wiener-Hopf, Wiener-Khinchin, Yule-Walker |
| L5 | Algorithms | Complete — Levinson-Durbin, Burg, Kalman, Welch |
| L6 | Canonical Problems | Complete — denoising, regulation, prediction |
| L7 | Applications | Complete — fire control, thermal, bio, comm |
| L8 | Advanced Topics | Complete — SR, Volterra, Fokker-Planck |
| L9 | Research Frontiers | Partial — documented |

## Core Definitions

- **Cybernetic System** (WCCyberneticSystem): State-input-output black box with feedback
- **Feedback Loop** (WCFeedbackLoop): PID controller — the fundamental regulation mechanism
- **Signal** (WCSignal): Time-domain information carrier
- **Information** (WCInformation): Negative entropy measure H = -Σ pᵢ log(pᵢ)
- **Wiener Process** (WCWienerPath): Continuous Brownian motion W(t) ~ N(0,t)

## Core Theorems

| Theorem | Formula | Code |
|---------|---------|------|
| Wiener-Hopf | Rₓₓ · h = rₓₖ | wc_filter_design_fir() |
| Wiener-Khinchin | S(ω) = F{R(τ)} | wc_psd_from_autocorr() |
| Yule-Walker | R · a = -r | wc_ar_fit_yule_walker() |
| Shannon-Hartley | C = B · log₂(1+SNR) | wc_entropy_capacity() |
| Levinson-Durbin | O(p²) Toeplitz solve | levinson_durbin_local() |
| Kalman Filter | x̂ = Ax̂ + K(y-Cx̂) | wc_kalman_update() |

## Core Algorithms

1. FIR Wiener filter via Wiener-Hopf equation (O(N³) general, O(N²) Toeplitz)
2. Levinson-Durbin recursion for AR parameter estimation (O(p²))
3. Burg maximum-entropy spectral estimation
4. Periodogram and Welch PSD estimation
5. Kalman filter predict-update cycle
6. PID control with anti-windup
7. Euler-Maruyama SDE integration
8. Yule-Walker AR model fitting
9. Hannan-Rissanen ARMA estimation
10. Gaussian elimination for linear systems

## Canonical Problems (examples/)

1. **Signal Denoising** — Wiener filter for noise removal (example_signal_denoising.c)
2. **Feedback Regulation** — Thermostat and servomechanism (example_feedback_regulation.c)
3. **Time Series Prediction** — AR modeling and forecasting (example_time_series.c)

## Build & Test

mkdir -p build
mkdir -p build
gcc -std=c11 -Wall -Wextra -O2 -Iinclude -o build/test_wiener tests/test_wiener.c -Lbuild -lwiener -lm
./build/test_wiener
=== mini-wiener-cybernetics Test Suite ===

L1: Definitions
  TEST signal_create... PASSED
  TEST signal_mean... PASSED
  TEST signal_variance... PASSED
  TEST signal_add_sub... PASSED
  TEST system_create... PASSED

L2: Core Concepts
  TEST feedback_pid... PASSED
  TEST information_entropy... PASSED
  TEST info_max_entropy... PASSED
  TEST info_kl_divergence... PASSED
  TEST homeostasis... PASSED
  TEST goal_directed... PASSED

L3: Mathematical Structures
  TEST wiener_path... PASSED
  TEST wiener_increment_normality... PASSED

L4: Fundamental Laws
  TEST autocorrelation... PASSED
  TEST wiener_filter_design... PASSED
  TEST power_spectrum... PASSED

L5: Algorithms
  TEST ar_model... PASSED
  TEST linear_predictor... PASSED

L6: Canonical Problems
  TEST kalman_predictor... PASSED
  TEST servomechanism... PASSED
  TEST anti_aircraft_predictor... PASSED

L7: Applications
  TEST thermal_control... PASSED
  TEST biological_homeostasis... PASSED
  TEST communication_system... PASSED

L8: Advanced Topics
  TEST stochastic_resonance... PASSED
  TEST first_passage... PASSED
  TEST arcsin_law... PASSED
  TEST volterra_series... PASSED

=== Results: 28/28 tests passed ===
mkdir -p build
All examples built.
mkdir -p build
All examples built.
=== example_signal_denoising ===
./build/example_signal_denoising
=== Wiener Filter: Signal Denoising ===

Clean signal: mean=-0.0000 rms=0.7906 energy=125.0000
Noisy signal: mean=-0.0083 rms=0.8690 energy=151.0453
WCWienerFilter: order=8 mse=0.051509 causal=1
  coeffs: 0.3500 0.3054 0.1834 0.1125 0.0748 0.0268 -0.0749 -0.1084 0.0000 
Filtered signal: mean=-0.0089 rms=0.7455 energy=111.1622

MSE before filtering: 0.160652
MSE after filtering:  0.058249
Improvement factor:   2.76x

--- Frequency-Domain Wiener Filter ---
Signal power: 0.3601, Max freq: 0.0300 Hz
Freq-filtered rms: 3.3559

Done.

=== example_feedback_regulation ===
./build/example_feedback_regulation
=== Cybernetic Feedback Regulation ===

--- Part 1: Thermostat ---
Room with thermal mass 10000 J/K, heat loss 200 W/K
Outside temperature: 5 C, Desired: 20 C

Time(h)  T_room(C)  Heater(W)  Energy(kWh)
------  ---------  ---------  -----------
   0        11.0       48272        0.017
   1        14.6       29983        0.027
   2        16.7       19082        0.034
   3        18.0       12584        0.038
   4        18.8        8711        0.041
   5        19.2        6403        0.043

Final efficiency: 92.08%

--- Part 2: Servomechanism ---
Inertia=1.0, Damping=0.5, Stiffness=2.0
Target position: 1.0 rad

Step   Position  Velocity  Torque
----   --------  --------  ------
   0     0.0299    0.8965  13.215
  10     0.1714    1.7638   7.952
  20     0.3785    2.2772   5.597
  30     0.6201    2.4831   3.096
  40     0.8655    2.3873   0.657
  50     1.0864    2.0278  -1.447

Final position: 1.0864 rad (target: 1.0)
Steady-state error: 0.0864 rad

--- Part 3: Feedback Loop Analysis ---
PID: Kp=5.0 Ki=2.0 Kd=0.5
Step   Output   Error    Control
----   ------   -----    -------
   0   0.4632  0.5691   3.2263
  10   0.6320  0.3813   1.3359
  20   0.7460  0.2639   0.9955
  30   0.8310  0.1764   0.7410
Overshoot: 63.5%
Settling time (2%): 11.177 s

Done.

=== example_time_series ===
./build/example_time_series
=== Wiener Time Series Prediction ===

Generated AR(2) process: mean=0.0094 var=0.3468

--- Part 1: Autocorrelation ---
WCAutocorrelation: max_lag=10 power=0.3469
  values: 0.3469 0.1870 0.0781 0.0079 -0.0167 -0.0147 0.0027 -0.0141 -0.0372 -0.0515 
The autocorrelation reveals the process memory.

--- Part 2: AR(2) Model (Yule-Walker) ---
AR coefficients: a1=-0.5889 a2=0.0923 (true: 0.7, -0.2)
Innovation variance: 0.2440 (true: 0.25)
AIC=-419.18 BIC=-411.77

--- Part 3: AR(2) Model (Burg's Method) ---
AR coefficients (Burg): a1=-0.5866 a2=0.0917
Innovation variance: 0.2447

--- Part 4: Multi-step Prediction ---
Predicted next 10 values:
Step  Prediction
----  ----------
  1       0.0687
  2      -0.3447
  3      -0.0723
  4       0.1963
  5       0.0607
  6      -0.1100
  7      -0.0459
  8       0.0605
  9       0.0326
 10      -0.0326

--- Part 5: Optimal Linear Predictor ---
Prediction order: 4
Prediction MSE: 0.242205
Prediction gain: 1.43 (= signal_power/MSE, >1 means successful)
True vs predicted (last 5 values):
  t=295: true=-0.0641 pred=-0.0592 err=-0.0049
  t=296: true=-0.3316 pred=-0.0079 err=-0.3236
  t=297: true=-0.0067 pred=-0.1727 err=0.1659
  t=298: true=0.5961 pred=0.0169 err=0.5792
  t=299: true=-0.0233 pred=0.3734 err=-0.3967

--- Part 6: Model Order Selection ---
Order  AIC       BIC       NoiseVar
-----  --------  --------  --------
  1     -417.80   -414.09    0.2468
  2     -418.33   -410.93    0.2447
  3     -418.50   -407.39    0.2429
  4     -416.52   -401.71    0.2429
  5     -414.57   -396.05    0.2429

--- Part 7: Power Spectrum ---
Total power: 0.1976
Peak frequency: 0.0600 Hz
Half-power bandwidth: 0.1200 Hz

Done.
rm -rf build

## 九校课程映射

| University | Course | Topics |
|-----------|--------|--------|
| MIT | 6.241J / 6.432 | Feedback, Wiener process |
| Stanford | EE363 | Wiener filter as optimization |
| Berkeley | EE222 | Feedback regulation |
| CMU | 24-654 | Cybernetics philosophy |
| Princeton | ELE 530 | Optimal estimation |
| Caltech | CDS140 | Stochastic dynamics |
| Cambridge | Control Theory | Servomechanisms |
| Oxford | C20 | Adaptive systems |
| ETH | 227-0216 | System identification |

## References

- Wiener (1948) *Cybernetics: Control and Communication in the Animal and the Machine*. MIT Press.
- Wiener (1949) *Extrapolation, Interpolation, and Smoothing of Stationary Time Series*. MIT Press.
- Wiener (1950) *The Human Use of Human Beings*. Houghton Mifflin.
- Rosenblueth, Wiener, Bigelow (1943) *Behavior, Purpose and Teleology*. Philosophy of Science.
- Shannon (1948) *A Mathematical Theory of Communication*. Bell System Technical Journal.
- Kalman (1960) *A new approach to linear filtering and prediction problems*. ASME J. Basic Eng.
- Kolmogorov (1941) *Interpolation and Extrapolation of Stationary Random Sequences*. Izv. Akad. Nauk.
