% Copyright (C) 2022 Fondazione Istitito Italiano di Tecnologia (IIT)
% All Rights Reserved.

classdef Types_Motor < Simulink.IntEnumType
    enumeration
        DC_Faulhaber_2342S_012_CR(0)
        DCEquivalent_Maxon_Variant1(1)
    end
    methods (Static = true)
        function retVal = addClassNameToEnumNames()
            retVal = true;
        end
    end    
end