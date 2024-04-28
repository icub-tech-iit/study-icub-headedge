% Copyright (C) 2022 Fondazione Istitito Italiano di Tecnologia (IIT)
% All Rights Reserved.

classdef Types_Mechanism < Simulink.IntEnumType
    enumeration
        Mk2(0)
        Mk3_Serial(1)
    end
    methods (Static = true)
        function retVal = addClassNameToEnumNames()
            retVal = true;
        end
    end    
end