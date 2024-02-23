% Copyright (C) 2022 Fondazione Istitito Italiano di Tecnologia (IIT)
% All Rights Reserved.

classdef Types_MotorDriver < Simulink.IntEnumType
    enumeration
        DC_Ideal_12V(0)
        DC_PWM_12V(1)
    end
    methods (Static = true)
        function retVal = addClassNameToEnumNames()
            retVal = true;
        end
    end    
end