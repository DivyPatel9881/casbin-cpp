#pragma once

#include "pch.h"

#include <direct.h>
#include <algorithm>
#include <fstream>

#include <enforcer.h>
#include <persist.h>
#include <config.h>
#include <util.h>
#include <exception.h>

using namespace std;

namespace test_management_api
{
    TEST_CLASS(TestManagementAPI)
    {
        public:

            string basic_example;
            Config* basic_config;

            TEST_METHOD_INITIALIZE(InitializeBasicConfig) {
                basic_example = filePath("../examples/basic_model.conf");
                basic_config = Config::NewConfig(basic_example);
            }

            string filePath(string filepath) {
                char* root = _getcwd(NULL, 0);
                string rootStr = string(root);

                vector <string> directories = Split(rootStr, "\\", -1);
                vector<string>::iterator it = find(directories.begin(), directories.end(), "x64");
                vector <string> left{ *(it - 1) };
                it = find_end(directories.begin(), directories.end(), left.begin(), left.end());
                int index = int(directories.size() + (it - directories.end()));

                vector <string> finalDirectories(directories.begin(), directories.begin() + index + 1);

                vector<string> userD = Split(filepath, "/", -1);
                for (int i = 1; i < userD.size(); i++)
                    finalDirectories.push_back(userD[i]);

                string filepath1 = finalDirectories[0];
                for (int i = 1; i < finalDirectories.size(); i++)
                    filepath1 = filepath1 + "/" + finalDirectories[i];
                return filepath1;
            }

            TEST_METHOD(TestGetList) {
                string model = filePath("../examples/rbac_model.conf");
                string policy = filePath("../examples/rbac_policy.csv");
                Enforcer* e = Enforcer :: NewEnforcer(model, policy);

                Assert::IsTrue(ArrayEquals(vector<string>{ "alice", "bob", "data2_admin" }, e->GetAllSubjects()));
                Assert::IsTrue(ArrayEquals(vector<string>{ "data1", "data2" }, e->GetAllObjects()));
                Assert::IsTrue(ArrayEquals(vector<string>{ "read", "write" }, e->GetAllActions()));
                Assert::IsTrue(ArrayEquals(vector<string>{ "data2_admin" }, e->GetAllRoles()));
            }

            void TestGetPolicy(Enforcer* e, vector<vector<string>> res) {
                vector<vector<string>> my_res;
                my_res = e->GetPolicy();

                int count = 0;
                for (int i = 0; i < my_res.size(); i++) {
                    for (int j = 0; j < res.size(); j++) {
                        if (ArrayEquals(my_res[i], res[j]))
                            count++;
                    }
                }

                if (count == res.size())
                    Assert::IsTrue(true);
            }

            void TestGetFilteredPolicy(Enforcer* e, int field_index, vector<vector<string>> res, vector<string> field_values) {
                vector<vector<string>> my_res = e->GetFilteredPolicy(field_index, field_values);
                for (int i = 0; i < res.size(); i++) {
                    Assert::IsTrue(ArrayEquals(my_res[i], res[i]));
                }
            }

            void TestGetGroupingPolicy(Enforcer* e, vector<vector<string>> res) {
                vector<vector<string>> my_res = e->GetGroupingPolicy();

                for (int i = 0; i < my_res.size(); i++) {
                    Assert::IsTrue(ArrayEquals(my_res[i], res[i]));
                }
            }

            void TestGetFilteredGroupingPolicy(Enforcer* e, int field_index, vector<vector<string>> res, vector<string> field_values) {
                vector<vector<string>> my_res = e->GetFilteredGroupingPolicy(field_index, field_values);

                for (int i = 0; i < my_res.size(); i++) {
                    Assert::IsTrue(ArrayEquals(my_res[i], res[i]));
                }
            }

            void TestHasPolicy(Enforcer* e, vector<string> policy, bool res) {
                bool my_res = e->HasPolicy(policy);
                Assert::AreEqual(res, my_res);
            }

            void TestHasGroupingPolicy(Enforcer* e, vector<string> policy, bool res) {
                bool my_res = e->HasGroupingPolicy(policy);
                Assert::AreEqual(res, my_res);
            }

            TEST_METHOD(TestGetPolicyAPI) {
                string model = filePath("../examples/rbac_model.conf");
                string policy = filePath("../examples/rbac_policy.csv");
                Enforcer* e = Enforcer::NewEnforcer(model, policy);

                TestGetPolicy(e, vector<vector<string>>{
                    {"alice", "data1", "read"},
                    { "bob", "data2", "write" },
                    { "data2_admin", "data2", "read" },
                    { "data2_admin", "data2", "write" }});

                TestGetFilteredPolicy(e, 0, vector<vector<string>>{ {"alice", "data1", "read"} }, vector<string>{"alice"});
                TestGetFilteredPolicy(e, 0, vector<vector<string>>{ {"bob", "data2", "write"}}, vector<string>{"bob"});
                TestGetFilteredPolicy(e, 0, vector<vector<string>>{ {"data2_admin", "data2", "read"}, { "data2_admin", "data2", "write" }}, vector<string>{"data2_admin"});
                TestGetFilteredPolicy(e, 1, vector<vector<string>>{ {"alice", "data1", "read"}}, vector<string>{"data1"});
                TestGetFilteredPolicy(e, 1, vector<vector<string>>{ {"bob", "data2", "write"}, { "data2_admin", "data2", "read" }, { "data2_admin", "data2", "write" }}, vector<string>{"data2"});
                TestGetFilteredPolicy(e, 2, vector<vector<string>>{ {"alice", "data1", "read"}, { "data2_admin", "data2", "read" }}, vector<string>{"read"});
                TestGetFilteredPolicy(e, 2, vector<vector<string>>{ {"bob", "data2", "write"}, { "data2_admin", "data2", "write" }}, vector<string>{"write"});

                TestGetFilteredPolicy(e, 0, vector<vector<string>>{ {"data2_admin", "data2", "read"}, { "data2_admin", "data2", "write" }}, vector<string>{"data2_admin", "data2"});
                // Note: "" (empty string) in fieldValues means matching all values.
                TestGetFilteredPolicy(e, 0, vector<vector<string>>{ {"data2_admin", "data2", "read"}}, vector<string>{"data2_admin", "", "read"});
                TestGetFilteredPolicy(e, 1, vector<vector<string>>{ {"bob", "data2", "write"}, { "data2_admin", "data2", "write" }}, vector<string>{"data2", "write"});

                TestHasPolicy(e, vector<string>{"alice", "data1", "read"}, true);
                TestHasPolicy(e, vector<string>{"bob", "data2", "write"}, true);
                TestHasPolicy(e, vector<string>{"alice", "data2", "read"}, false);
                TestHasPolicy(e, vector<string>{"bob", "data3", "write"}, false);

                TestGetGroupingPolicy(e, vector<vector<string>>{ {"alice", "data2_admin"}});

                TestGetFilteredGroupingPolicy(e, 0, vector<vector<string>>{ {"alice", "data2_admin"}}, vector < string>{"alice"});
                TestGetFilteredGroupingPolicy(e, 0, vector<vector<string>>{}, vector < string>{"bob"});
                TestGetFilteredGroupingPolicy(e, 1, vector<vector<string>>{}, vector<string>{"data1_admin"});
                TestGetFilteredGroupingPolicy(e, 1, vector<vector<string>>{ {"alice", "data2_admin"}}, vector<string>{"data2_admin"});
                // Note: "" (empty string) in fieldValues means matching all values.
                TestGetFilteredGroupingPolicy(e, 0, vector<vector<string>>{ {"alice", "data2_admin"}}, vector<string>{"", "data2_admin"});

                TestHasGroupingPolicy(e, vector<string>{"alice", "data2_admin"}, true);
                TestHasGroupingPolicy(e, vector<string>{"bob", "data2_admin"}, false);
            }


            TEST_METHOD(TestModifyPolicyAPI) {
                string model = filePath("../examples/rbac_model.conf");
                string policy = filePath("../examples/rbac_policy.csv");
                Adapter* adapter = BatchFileAdapter::NewAdapter(policy);
                Enforcer* e = Enforcer::NewEnforcer(model, adapter);

                TestGetPolicy(e, vector<vector<string>>{
                    {"alice", "data1", "read"},
                    { "bob", "data2", "write" },
                    { "data2_admin", "data2", "read" },
                    { "data2_admin", "data2", "write" }});

                e->RemovePolicy(vector<string>{"alice", "data1", "read"});
                e->RemovePolicy(vector<string>{"bob", "data2", "write"});
                e->RemovePolicy(vector<string>{"alice", "data1", "read"});
                e->AddPolicy(vector<string>{"eve", "data3", "read"});
                e->AddPolicy(vector<string>{"eve", "data3", "read"});

                vector<vector<string>>rules{
                    {"jack", "data4", "read"},
                    {"katy", "data4", "write"},
                    {"leyo", "data4", "read"},
                    {"ham", "data4", "write"},
                };

                e->AddPolicies(rules);
                e->AddPolicies(rules);

                TestGetPolicy(e, vector<vector<string>>{
                    {"data2_admin", "data2", "read"},
                    { "data2_admin", "data2", "write" },
                    { "eve", "data3", "read" },
                    { "jack", "data4", "read" },
                    { "katy", "data4", "write" },
                    { "leyo", "data4", "read" },
                    { "ham", "data4", "write" }});

                e->RemovePolicies(rules);
                e->RemovePolicies(rules);

                vector<string>named_policy{ "eve", "data3", "read" };
                e->RemoveNamedPolicy("p", named_policy);
                e->AddNamedPolicy("p", named_policy);

                TestGetPolicy(e, vector<vector<string>>{
                    {"data2_admin", "data2", "read"},
                    { "data2_admin", "data2", "write" },
                    { "eve", "data3", "read" }});

                e->RemoveFilteredPolicy(1, vector<string>{"data2"});

                TestGetPolicy(e, vector<vector<string>>{ {"eve", "data3", "read"}});
            }

            TEST_METHOD(TestModifyGroupingPolicyAPI) {
                string model = filePath("../examples/rbac_model.conf");
                string policy = filePath("../examples/rbac_policy.csv");
                Adapter* adapter = BatchFileAdapter::NewAdapter(policy);
                Enforcer* e = Enforcer::NewEnforcer(model, adapter);

                Assert::IsTrue(ArrayEquals(vector<string>{"data2_admin"}, e->GetRolesForUser("alice", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{}, e->GetRolesForUser("bob", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{}, e->GetRolesForUser("eve", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{}, e->GetRolesForUser("non_exist", vector<string>{})));

                e->RemoveGroupingPolicy(vector<string>{"alice", "data2_admin"});
                e->AddGroupingPolicy(vector<string>{"bob", "data1_admin"});
                e->AddGroupingPolicy(vector<string>{"eve", "data3_admin"});

                vector<vector<string>> grouping_rules{
                    {"ham", "data4_admin"},
                    {"jack", "data5_admin"},
                };

                e->AddGroupingPolicies(grouping_rules);
                Assert::IsTrue(ArrayEquals(vector<string>{"data4_admin"}, e->GetRolesForUser("ham", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{"data5_admin"}, e->GetRolesForUser("jack", vector<string>{})));                
                e->RemoveGroupingPolicies(grouping_rules);

                Assert::IsTrue(ArrayEquals(vector<string>{}, e->GetRolesForUser("alice", vector<string>{})));
                vector<string> named_grouping_policy{ "alice", "data2_admin" };
                Assert::IsTrue(ArrayEquals(vector<string>{}, e->GetRolesForUser("alice", vector<string>{})));
                e->AddNamedGroupingPolicy("g", named_grouping_policy);
                Assert::IsTrue(ArrayEquals(vector<string>{"data2_admin"}, e->GetRolesForUser("alice", vector<string>{})));
                e->RemoveNamedGroupingPolicy("g", named_grouping_policy);

                e->AddNamedGroupingPolicies("g", grouping_rules);
                e->AddNamedGroupingPolicies("g", grouping_rules);
                Assert::IsTrue(ArrayEquals(vector<string>{"data4_admin"}, e->GetRolesForUser("ham", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{"data5_admin"}, e->GetRolesForUser("jack", vector<string>{})));
                e->RemoveNamedGroupingPolicies("g", grouping_rules);
                e->RemoveNamedGroupingPolicies("g", grouping_rules);

                Assert::IsTrue(ArrayEquals(vector<string>{}, e->GetRolesForUser("alice", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{"data1_admin"}, e->GetRolesForUser("bob", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{"data3_admin"}, e->GetRolesForUser("eve", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{}, e->GetRolesForUser("non_exist", vector<string>{})));

                Assert::IsTrue(ArrayEquals(vector<string>{"bob"}, e->GetUsersForRole("data1_admin", vector<string>{})));
                try {
                    e->GetUsersForRole("data2_admin", vector<string>{});
                }
                catch (CasbinRBACException e) {
                    Assert::IsTrue(true);
                }
                Assert::IsTrue(ArrayEquals(vector<string>{"eve"}, e->GetUsersForRole("data3_admin", vector<string>{})));
                
                e->RemoveFilteredGroupingPolicy(0, vector<string>{"bob"});

                Assert::IsTrue(ArrayEquals(vector<string>{}, e->GetRolesForUser("alice", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{}, e->GetRolesForUser("bob", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{"data3_admin"}, e->GetRolesForUser("eve", vector<string>{})));
                Assert::IsTrue(ArrayEquals(vector<string>{}, e->GetRolesForUser("non_exist", vector<string>{})));

                try {
                    e->GetUsersForRole("data1_admin", vector<string>{});
                }
                catch (CasbinRBACException e) {
                    Assert::IsTrue(true);
                }
                try {
                    e->GetUsersForRole("data2_admin", vector<string>{});
                }
                catch (CasbinRBACException e) {
                    Assert::IsTrue(true);
                }
                Assert::IsTrue(ArrayEquals(vector<string>{"eve"}, e->GetUsersForRole("data3_admin", vector<string>{})));
            }
    };
}