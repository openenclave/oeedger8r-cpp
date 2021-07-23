// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

import groovy.json.JsonSlurper

def allowedCommitters = [
    'anakrish@microsoft.com',
    'brmclare@microsoft.com',
    'chrisyan@microsoft.com',
    'marupire@microsoft.com',
    'mishih@microsoft.com',
    'rosan@microsoft.com'
]

String IGNORED_DIRS = "^(docs)|\\.md\$"

def cmakeBuildoeedger8r(String BUILD_CONFIG, String COMPILER) {
    /* Build oeedger8r based on build config, compiler and platform */
    cleanWs()
    checkout([$class: 'GitSCM',
             branches: [[name: BRANCH_NAME]],
             extensions: [],
             userRemoteConfigs: [[url: 'https://github.com/openenclave/oeedger8r-cpp']]])
    dir ('build') {
        if (isUnix()) {
            sh """
            echo COMPILER IS ${COMPILER}
            """
            switch(COMPILER) {
                case COMPILER.contains("clang-"):
                    compiler_version = COMPILER.split('-').get(1)
                    c_compiler = "clang-${compiler_version}"
                    cpp_compiler = "clang++-${compiler_version}"
                    break
                case "gcc":
                    c_compiler = "gcc"
                    cpp_compiler = "g++"
                    break
                default:
                    // This is needed for backwards compatibility with the old
                    // implementation of the method.
                    c_compiler = "clang-10"
                    cpp_compiler = "clang++-10"
            }
            withEnv(["CC=${c_compiler}","CXX=${cpp_compiler}"]) {
                sh """
                cmake .. -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_CONFIG} -Wdev
                ninja -v
                ctest --output-on-failure --timeout
                """
            }
        } else {
            bat (
                returnStdout: false,
                returnStatus: false,
                script: """
                        call vcvars64.bat x64
                        setlocal EnableDelayedExpansion
                        cmake.exe .. -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_CONFIG} -Wdev || exit !ERRORLEVEL!
                        ninja -v -j 4 || exit !ERRORLEVEL!
                        ctest.exe -V --output-on-failure || exit !ERRORLEVEL!
                        """
            )
        }
    }
}

pipeline {
    options {
        timeout(time: 30, unit: 'MINUTES')
    }
    agent {
        label 'nonSGX-ubuntu-2004'
    }
    stages {
        stage("Pre-flight checks") {
            stages {
                stage("Check commit signature") {
                    when { 
                        not {
                            anyOf {
                                branch 'master';
                                branch 'staging';
                                branch 'trying'
                            }
                        }
                    }
                    steps {
                        cleanWs()
                        checkout([
                            $class: 'GitSCM',
                            branches: scm.branches,
                            doGenerateSubmoduleConfigurations: false,
                            extensions: [[$class: 'SubmoduleOption',
                                            disableSubmodules: true,
                                            recursiveSubmodules: false,
                                            trackingSubmodules: false]], 
                            submoduleCfg: [],
                            userRemoteConfigs: scm.userRemoteConfigs
                        ])
                        script {
                            commitHash = sh(
                                script: 'git rev-parse HEAD',
                                returnStdout: true
                            ).trim()
                            commitVerificationJSON = sh (
                                script: """
                                        curl https://api.github.com/repos/openenclave/oeedger8r-cpp/commits/${commitHash}
                                        """,
                                returnStdout: true
                            ).trim()
                            def jsonSlurper = new JsonSlurper()
                            def commitObject = jsonSlurper.parseText(commitVerificationJSON)
                            Boolean verifiedCommit = commitObject.commit.verification.verified
                            if (verifiedCommit) {
                                assert allowedCommitters.contains(commitObject.commit.author.email)
                            } else {
                                error("Commit failed verification. Is it signed?")
                            }
                        }
                    }
                }
                stage("Compare changes") {
                    steps {
                        cleanWs()
                        checkout([
                            $class: 'GitSCM',
                            branches: scm.branches + [[name: '*/master']],
                            doGenerateSubmoduleConfigurations: false,
                            extensions: [[$class: 'SubmoduleOption',
                                            disableSubmodules: true,
                                            recursiveSubmodules: false,
                                            trackingSubmodules: false]], 
                            submoduleCfg: [],
                            userRemoteConfigs: scm.userRemoteConfigs
                        ])
                        script {
                            // Check if git diff vs origin/master contains changes outside of ignored directories
                            gitChanges = sh (
                                script: """
                                        git diff --name-only HEAD origin/master | grep --invert-match --extended-regexp \'${IGNORED_DIRS}\' || true
                                        """,
                                returnStdout: true
                            ).trim()
                        }
                    }
                }
            }
        }
        stage('Parallel tests') {
            parallel {
                stage('Ubuntu 20.04 RelWithDebInfo') { steps { node("nonSGX-ubuntu-2004")  { cmakeBuildoeedger8r("RelWithDebInfo", "clang-10")}}}
                stage('Ubuntu 20.04 Debug')          { steps { node("nonSGX-ubuntu-2004")  { cmakeBuildoeedger8r("Debug",          "clang-10")}}}
                stage('Ubuntu 18.04 RelWithDebInfo') { steps { node("nonSGX-ubuntu-1804")  { cmakeBuildoeedger8r("RelWithDebInfo", "clang-10")}}}
                stage('Ubuntu 18.04 Debug')          { steps { node("nonSGX-ubuntu-1804")  { cmakeBuildoeedger8r("Debug",          "clang-10")}}}
                stage('WS 2019 RelWithDebInfo')      { steps { node("nonSGX-Windows-2019") { cmakeBuildoeedger8r("RelWithDebInfo", "clang-10")}}}
                stage('WS 2019 Debug')               { steps { node("nonSGX-Windows-2019") { cmakeBuildoeedger8r("Debug",          "clang-10")}}}
            }
        }
    }
    post ('Clean Up') {
        always{
            cleanWs()
        }
    }
}
