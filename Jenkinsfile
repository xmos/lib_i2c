// This file relates to internal XMOS infrastructure and should be ignored by external users

@Library('xmos_jenkins_shared_library@v0.34.0') _

def clone_test_deps() {
    dir("${WORKSPACE}") {
        sh "git clone git@github.com:xmos/test_support"
        sh "git -C test_support checkout v2.0.0"
    }
}

def checkout_shallow()
{
    checkout scm: [
        $class: 'GitSCM',
        branches: scm.branches,
        userRemoteConfigs: scm.userRemoteConfigs,
        extensions: [[$class: 'CloneOption', depth: 1, shallow: true, noTags: false]]
    ]
}

def archiveLib(String repoName) {
    sh "git -C ${repoName} clean -xdf"
    sh "zip ${repoName}_sw.zip -r ${repoName}"
    archiveArtifacts artifacts: "${repoName}_sw.zip", allowEmptyArchive: false
}

getApproval()

pipeline {
  agent none
  environment {
    REPO = 'lib_i2c'
  }
  options {
    buildDiscarder(xmosDiscardBuildSettings())
    skipDefaultCheckout()
    timestamps()
  }
  parameters {
    string(
      name: 'TOOLS_VERSION',
      defaultValue: '15.3.0',
      description: 'The XTC tools version'
    )
    string(
      name: 'XMOSDOC_VERSION',
      defaultValue: 'v6.1.2',
      description: 'The xmosdoc version'
    )
    string(
      name: 'INFR_APPS_VERSION',
      defaultValue: 'develop',
      description: 'The infr_apps version'
    )
  }
  stages {
    stage('Build and test') {
      agent {
        label 'documentation && linux && x86_64'
      }
      stages {
        stage('Build examples') {
          steps {
            println "Stage running on ${env.NODE_NAME}"

            dir("${REPO}") {
              checkout_shallow()

              dir("examples") {
                withTools(params.TOOLS_VERSION) {
                  sh "cmake -G 'Unix Makefiles' -B build -DDEPS_CLONE_SHALLOW=TRUE"
                  sh 'xmake -C build -j 8'
                }
              }
            } // dir("${REPO}")
          } // steps
        }  // stage('Build examples')

        stage('Library checks') {
          steps {
            warnError("Library checks failed") {
                runLibraryChecks("${WORKSPACE}/${REPO}", "${params.INFR_APPS_VERSION}")
            }
          } // steps
        } // stage('Library Checks')

        stage('Build documentation') {
          steps {
            dir("${REPO}") {
              warnError("Documentation build failed") {
                buildDocs()
                dir("examples/AN00156_i2c_master_example") {
                  buildDocs()
                }
                dir("examples/AN00157_i2c_slave_example") {
                  buildDocs()
                }
              }
            } // dir("${REPO}")
          } // steps
        } // stage('Build Documentation')

        stage('Simulator tests') {
          steps {
            dir("${REPO}") {
              withTools(params.TOOLS_VERSION) {
                clone_test_deps()
                dir("tests") {
                  createVenv(reqFile: "requirements.txt")
                  withVenv {
                    sh "cmake -G 'Unix Makefiles' -B build -DDEPS_CLONE_SHALLOW=TRUE"
                    sh 'xmake -C build -j 8'
                    sh "pytest -v -n auto --junitxml=pytest_result.xml"
                    junit "pytest_result.xml"
                  } // withVenv
                } // dir("tests")
              } //withTools
            } // dir("${REPO}")
          } // steps
        } // Simulator tests

        stage("Archive lib") {
            steps
            {
                archiveLib(REPO)
            }
        }
      } // stages
      post {
        cleanup {
            xcoreCleanSandbox()
        }
      }
    }  // Build and test
  }
}
