clear all
clc

Hz = 64     %   Gateway polling rate

M = csvread('logfile190524c.csv', 1, 0);    %   read file skipping first line

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

%   display load cells (Channel 2 disconnected here)
figure(1)
clf
plot(t, M(:, 2:2) - mean(M(1:600, 2:2)), 'linewidth', 1.5)
xlabel('Time (s)')
ylabel('Torque (Nm)')
title('Torque Sensors')
legend('Running Torque Sensor', 'Static Torque Sensor', 'location', 'best')

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
grid on
zoom on

%   display HV
figure(3)
clf
subplot(2, 1, 1)
plot(t, M(:, 9), 'linewidth', 1.5)
title('HV Battery')
ylabel('Voltage (V)')
grid on
zoom on
subplot(2, 1, 2)
plot(t, M(:, 10), 'linewidth', 1.5)
ylabel('Current (A)')
xlabel('Time (s)')
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
grid on
zoom on
subplot(2, 1, 2)
plot(t, M(:, [14 16]), 'linewidth', 1.5)
ylabel('Current (A)')
xlabel('Time (s)')
legend('D', 'Q', 'location', 'best')
grid on
zoom on

%   display reported and commanded torques and speed
figure(6)
clf
subplot(2,1,1)
% plot(t, M(:, [17:19 22:24]), 'linewidth', 1.5)
plot(t, [M(:, 17)/10, (M(:, 2:2)- mean(M(1:600, 2:2))) * 30,...
    cz  M(:, 22:24)/10], 'linewidth', 1.5)   %   temp plot
title('Torques')
ylabel('Torque (Nm)')
legend('Reported Torque', 'Running Torque', '?',...
    'Maximum Command', 'Minimum Command', 'Standby', 'location', 'northeast')
grid on
zoom on

subplot(2,1,2)
plot(t, M(:, 11), 'linewidth', 1.5)
title('Speed')
ylabel('Speed (RPM)')
xlabel('Time (s)')
grid on
zoom on

figure(7)
clf
plot(t, M(:, 22:24), 'linewidth', 1.5)
title('Commanded Torques')
xlabel('Time (s)')
ylabel('Torque (Nm)')

legend('Min', 'Max',...
    'Standby', 'location', 'best')
grid on
zoom on







