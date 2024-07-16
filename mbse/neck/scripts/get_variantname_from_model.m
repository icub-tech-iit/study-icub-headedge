function variantname = get_variantname_from_model(mdl)
% Copyright (C) 2024 Fondazione Istitito Italiano di Tecnologia (IIT)
% All Rights Reserved.

    isLoaded = bdIsLoaded(mdl);
    if ~isLoaded
        load_system(mdl);
    end

    vars = Simulink.VariantManager.findVariantControlVars(bdroot);
    variantname = "Variant_Neck";
    
    for i = 1:length(vars)
        if vars(i).Name == "Selector_Mechanism"
            if vars(i).Value.Value == "Mk3_Serial"
                variantname = strcat(variantname, "Mk3_Serial");
            else
                variantname = strcat(variantname, "Mk2");
            end
            break;
        end
    end

    variantname = strcat(variantname, "_");

    for i = 1:length(vars)
        if vars(i).Name == "Selector_Payload"
            if vars(i).Value.Value == "ergoCubHead"
                variantname = strcat(variantname, "ergoCub");
            elseif vars(i).Value.Value == "ergoCubHeadWithEyes"
                variantname = strcat(variantname, "ergoCubWithEyes");    
            else
                variantname = strcat(variantname, "iCub");
            end
            break;
        end
    end

    if ~isLoaded
        close_system(mdl, 0);
    end
end
