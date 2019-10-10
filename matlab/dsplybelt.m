clear all
clc

%   This log was to explore the relaxation time for the belt. Initial value
%   is only supporting motor and its ancillary components. The load binder
%   is applied and the relaxation behavior of the belt stretch can be
%   studied.

format compact
Hz = 64     %   Gateway polling rate
MmtArm = 0.6    %   Static Torque moment arm
TG = 53/32      %   Torque gain to output shaft; ratio of pulleys used
%   consolidated conversion factor from daN to torque (Nm) in static torque
%   sensing system
STCF = MmtArm * 10 / TG;

%   Load cell gain correction factors from calibration experiments.
daNDyn380SclCrrtnt = 2.0577 %   Scale correction factor for Dynamic Torque 
                            %   500 kg load cell on Channel 1 to daN
daNPod382SclCrrtnt = 0.9353 %   Scale correction factor for Pod2 load cell
                            %   on Channel 2 to daN

%   Dynamic Torque load cell offset
Dyn380HngFr = -1.6258

M = csvread('logfile190610belt.csv', 1, 0);    %   read file skipping first line



%   make entries before first payload Nans
for i = 1:size(M,2)
    indx = find(M(:, i));
    if ~isempty(indx) 
        indx = indx(1);
    end    
    M(1:(indx - 1), i) = NaN;
end

%   unwrap time values
t = (round(unwrap((M(:, 1) - Hz/2)...
    *(2 * pi / Hz )) * Hz/ (2 * pi) + Hz / 2) - M(1, 1)) / Hz;

cz = zeros(size(M, 1), 1);  % dummy zero column

Tstrt = 0
Tstp = size(t, 1) / 64
TZ = [55 * Hz: 65 * Hz];         %   span over which torques are zeroed
DYN = 0.69

%   display load cells 
figure(1)
clf
plot(t, [(M(:, 2) - Dyn380HngFr) * (daNDyn380SclCrrtnt) , ...
    (M(:, 3) - mean(M(TZ, 3)) ) * daNPod382SclCrrtnt * STCF],...
    'linewidth', 1.5)
xlabel('Time (s)')
ylabel('Dyanic Load Cell (daN), Torque (Nm)')
title('Torque Sensors')
legend('Running Torque Sensor (daN)', 'Static Torque Sensor (Nm)', 'location', 'south')
xlim([Tstrt Tstp])
grid on
zoom on

%   Rest of plots were removed as no torque pulses employed.








