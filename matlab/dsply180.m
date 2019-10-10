clear all
clc

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

M = csvread('logfile190610180.csv', 1, 0);    %   read file skipping first line



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
Tstp = size(t, 1)/64
TZ = [31 * Hz: 32 * Hz];         %   span over which torques are zeroed
DYN = 0.76

%   display load cells 
figure(1)
clf
plot(t, [(M(:, 2) - Dyn380HngFr) * (daNDyn380SclCrrtnt) , ...
    (M(:, 3) - mean(M(TZ, 3)) ) * daNPod382SclCrrtnt * STCF],...
    'linewidth', 1.5)
xlabel('Time (s)')
ylabel('Dyanic Load Cell (daN), Torque (Nm)')
title('Torque Sensors')
legend('Running Torque Sensor (daN)', 'Static Torque Sensor (Nm)', 'location', 'northeast')
xlim([Tstrt Tstp])
grid on
zoom on

%   display temperatures
figure(2)
clf
plot(t, M(:, 6:8), 'linewidth', 1.5)
xlabel('Time (s)')
ylabel('Temperature (C)')
title('DMOC/Motor Temperatures')
legend('Rotor', 'Inverter',...
    'Stator', 'location', 'best')
xlim([Tstrt Tstp])
ylim([10 40])
grid on
zoom on

%   display HV
figure(3)
clf

plot(t, [M(:, 9:10) M(:, 9) .* M(:, 10)/1e3], 'linewidth', 1.5)
title('HV Battery')
ylabel('Voltage (V), Current (A), Power (kW)')
grid on
zoom on
legend('Voltage', 'Current', 'Power', 'location', 'east')
xlabel('Time (s)')
xlim([Tstrt Tstp])
grid on
zoom on

%   display speed and status
figure(4)
clf
plot(t, M(:, 11:12), 'linewidth', 1.5)
title('Speed and Status')
legend('Speed', 'DMOC Status', 'location', 'best')
ylabel('Reported Speed (RPM)')
xlabel('Time (s)')
ylim([-20 10])
xlim([Tstrt Tstp])
grid on
zoom on

%   display D and Q values
figure(5)
clf
subplot(2, 1, 1)
plot(t, M(:, [13 15]), 'linewidth', 1.5)
title('D & Q Values')
ylabel('Volts')
legend('D', 'Q', 'location', 'best')
xlim([Tstrt Tstp])
grid on
zoom on
subplot(2, 1, 2)
plot(t, M(:, [14 16]), 'linewidth', 1.5)
ylabel('Current (A)')
xlabel('Time (s)')
legend('D', 'Q', 'location', 'best')
xlim([Tstrt Tstp])
grid on
zoom on

%   display reported and commanded torques and speed
figure(6)
clf
% plot(t(ixstrt:ixstp, M(:, [17:19 22:24]), 'linewidth', 1.5)

plot(t, [150/150*M(:, 17)/10, ...
    (M(:, 2)- mean(M(TZ, 2))) * (daNDyn380SclCrrtnt * 10 * DYN),...
    (M(:, 3)- mean(M(TZ, 3)))* (daNPod382SclCrrtnt * STCF), ...
    M(:, 22:24)/10], 'linewidth', 1.5)   %   temp plot

title('Torques')
ylabel('Torque (Nm)')
legend('Reported Torque', 'Running Torque', 'Static Torque',...
    'Maximum Command', 'Minimum Command', 'Standby', 'location', 'northwest')
xlim([Tstrt Tstp])
grid on
zoom on









