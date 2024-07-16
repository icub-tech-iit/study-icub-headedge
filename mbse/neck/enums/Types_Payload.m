% Copyright (C) 2022 Fondazione Istitito Italiano di Tecnologia (IIT)
% All Rights Reserved.

classdef Types_Payload < Simulink.IntEnumType
    enumeration
        iCubHead(0)
        ergoCubHead(1)
        ergoCubHeadWithEyes(2)
    end
    methods (Static = true)
        function retVal = addClassNameToEnumNames()
            retVal = true;
        end
    end    
end