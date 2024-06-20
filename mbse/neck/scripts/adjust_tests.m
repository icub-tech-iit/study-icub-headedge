function adjust_tests(testFile, minJerk_T)
% Copyright (C) 2024 Fondazione Istitito Italiano di Tecnologia (IIT)
% All Rights Reserved.

    tf = sltest.testmanager.TestFile(testFile);
    setVariantConf(tf, "Test Neck Mk2 ergoCub", "Variant_NeckMk2_ergoCub", minJerk_T);
    setVariantConf(tf, "Test Neck Mk2 iCub", "Variant_NeckMk2_iCub", minJerk_T);
    setVariantConf(tf, "Test Neck Mk3 Serial ergoCub", "Variant_NeckMk3_Serial_ergoCub", minJerk_T);
    setVariantConf(tf, "Test Neck Mk3 Serial iCub", "Variant_NeckMk3_Serial_iCub", minJerk_T);
    saveToFile(tf);

end

function setVariantConf(tf, tsName, VariantConfName, minJerk_T)
    ts = getTestSuiteByName(tf, tsName);
    tc = getTestCases(ts);
    for i = 1:length(tc)
        setProperty(tc(i), "VariantConfiguration", VariantConfName);
        
        if contains(tc(i).Name, 'High Performance') 
            ps = getParameterSets(tc(i));
            for j = 1:length(ps)
                addParameterOverride(ps(j), 'T', minJerk_T, BlockPath='arch_logical/MCU/Planner/Planner MinJerk/Minimum Jerk');
            end
        end
    end
end
