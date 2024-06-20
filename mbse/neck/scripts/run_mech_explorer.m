function run_mech_explorer(variant, varargin)
% Copyright (C) 2024 Fondazione Istitito Italiano di Tecnologia (IIT)
% All Rights Reserved.
%#ok<*AGROW>
%#ok<*NASGU>

    if nargin <= 2
        S1 = dir('./cache');
        S1 = S1(~[S1.isdir]);
        S2 = [];
        for i = 1:length(S1)
            if contains(S1(i).name, 'voltage')
                S2 = [S2; S1(i)];
            end
        end
        [~, idx] = sort([S2.datenum]);
        filename = ['./cache/' S2(idx(end)).name];
    end
    
    hidden = false;
    if nargin >= 2
        hidden = strcmpi(varargin{1}, "hidden");
    end
    
    if nargin >= 3
        filename = varargin{2};
    end
    
    orig_filename = './cache/voltage.mat';
    if ~strcmpi(filename, orig_filename)
        copyfile(filename, orig_filename);
    end
    data = load(orig_filename);
    Tend = data.voltage.Time(end);
    
    mdl = 'PlaybackMechExplorer';
    if hidden
        load_system(mdl);
    else
        open_system(mdl);
    end
    
    Simulink.VariantManager.activateModel(mdl, Configuration=variant);
    set_param(mdl, 'StopTime', num2str(Tend));
    
    if hidden
        set_param(mdl, 'SimulationCommand', 'Update');
        pause(3);
        sim(mdl);
        pause(3);
        close_system(mdl, 0);
    end

end
