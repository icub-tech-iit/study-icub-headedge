%%
clear
clc
close all

%%
prefix = 'test_velocity_';
file_list = dir(fullfile(string(pwd), [prefix,'*']));

for i = 1:numel(file_list)
    filename = fullfile(string(pwd), file_list(i).name);
    load(file_list(i).name);
    vel = string(extractBetween(file_list(i).name, prefix, ".mat"));

    figure
    tl = tiledlayout(3,1);
    time = test_velocity.joints_state.positions.timestamps - test_velocity.joints_state.positions.timestamps(1,1);

    neck_pitch_pos = test_velocity.joints_state.positions.data(1,:);
    neck_pitch_vel = test_velocity.joints_state.velocities.data(1,:);
    neck_pitch_ref_pos = test_velocity.PIDs.position_reference.data(1,:);
    
    nexttile
    plot(time, neck_pitch_pos, 'LineWidth', 1);
    hold on
    grid on
    plot(time, neck_pitch_ref_pos, 'LineWidth', 1);
    xlabel("Time [s]");
    ylabel("Position neck pitch [deg]");
    xlim([0, time(end)]);
    ylim([-45, 22]);
    legend("Position feedback","Position reference")
    
    neck_roll_pos = test_velocity.joints_state.positions.data(2,:);
    neck_roll_vel = test_velocity.joints_state.velocities.data(2,:);
    neck_roll_ref_pos = test_velocity.PIDs.position_reference.data(2,:);
    
    nexttile
    plot(time, neck_roll_pos, 'LineWidth', 1);
    hold on
    grid on
    plot(time, neck_roll_ref_pos, 'LineWidth', 1);
    xlabel("Time [s]");
    ylabel("Position neck roll [deg]");
    xlim([0, time(end)]);
    ylim([-20, 20]);
    legend("Position feedback","Position reference")
    
    neck_yaw_pos = test_velocity.joints_state.positions.data(3,:);
    neck_yaw_vel = test_velocity.joints_state.velocities.data(3,:);
    neck_yaw_ref_pos = test_velocity.PIDs.position_reference.data(3,:);

    nexttile
    plot(time, neck_yaw_pos, 'LineWidth', 1);
    hold on
    grid on
    plot(time, neck_yaw_ref_pos, 'LineWidth', 1);
    xlabel("Time [s]");
    ylabel("Position neck yaw[deg]");
    xlim([0, time(end)]);
    ylim([-45, 45]);
    legend("Position feedback","Position reference")
    title(tl, "Positions with " + vel + " deg/s");

    figure
    plot(time, neck_pitch_vel, 'LineWidth', 1);
    hold on
    grid on
    xlabel("Time [s]");
    ylabel("Velocity [deg/s]");
    xlim([0, time(end)]);
    plot(time, neck_roll_vel, 'LineWidth', 1);
    plot(time, neck_yaw_vel, 'LineWidth', 1);
    title("Velocities with " + vel + " deg/s");
    legend("neck pitch", "neck roll", "neck yaw")
end

