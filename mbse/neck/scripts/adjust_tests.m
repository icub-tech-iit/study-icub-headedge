function adjust_tests(testFile)
% Copyright (C) 2024 Fondazione Istitito Italiano di Tecnologia (IIT)
% All Rights Reserved.

    tf = sltest.testmanager.TestFile(testFile);
    setVariantConf(tf, "Test Neck Mk2 ergoCub", "Variant_NeckMk2_ergoCub");
    setVariantConf(tf, "Test Neck Mk2 iCub", "Variant_NeckMk2_iCub");
    setVariantConf(tf, "Test Neck Mk3 Serial ergoCub", "Variant_NeckMk3_Serial_ergoCub");
    setVariantConf(tf, "Test Neck Mk3 Serial iCub", "Variant_NeckMk3_Serial_iCub");
    saveToFile(tf);

end

function setVariantConf(tf, tsName, VariantConfName)
    ts = getTestSuiteByName(tf, tsName);
    tc = getTestCases(ts);
    for i = 1:length(tc)
        setProperty(tc(i), "VariantConfiguration", VariantConfName);
    end
end
