// This file relates to internal XMOS infrastructure and should be ignored by external users

@Library('xmos_jenkins_shared_library@v0.39.0') _

def clone_test_deps() {
    dir("${WORKSPACE}") {
        sh "git clone git@github.com:xmos/test_support"
        sh "git -C test_support checkout v2.1.0"
    }
}

getApproval()

pipeline {
  agent none
  environment {
    REPO_NAME = 'lib_i2c'
  }
  options {
    buildDiscarder(xmosDiscardBuildSettings())
    skipDefaultCheckout()
    timestamps()
  }
  parameters {
    string(
      name: 'TOOLS_VERSION',
      defaultValue: '15.3.1',
      description: 'The XTC tools version'
    )
    string(
      name: 'XMOSDOC_VERSION',
      defaultValue: 'v7.3.0',
      description: 'The xmosdoc version'
    )
    string(
      name: 'INFR_APPS_VERSION',
      defaultValue: 'v2.1.0',
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

            dir("${REPO_NAME}") {
              checkoutScmShallow()

              dir("examples") {
                withTools(params.TOOLS_VERSION) {
                  xcoreBuild()
                }
              }
            } // dir("${REPO_NAME}")
          } // steps
        }  // stage('Build examples')

        stage('Library checks') {
          steps {
            warnError("Library checks failed") {
                runLibraryChecks("${WORKSPACE}/${REPO_NAME}", "${params.INFR_APPS_VERSION}")
            }
          } // steps
        } // stage('Library Checks')

        stage('Build documentation') {
          steps {
            dir("${REPO_NAME}") {
              warnError("Documentation build failed") {
                buildDocs()
                dir("examples/AN00156_i2c_master_example") {
                  buildDocs()
                }
                dir("examples/AN00157_i2c_slave_example") {
                  buildDocs()
                }
              }
            } // dir("${REPO_NAME}")
          } // steps
        } // stage('Build Documentation')

        stage('Simulator tests') {
          steps {
            dir("${REPO_NAME}") {
              withTools(params.TOOLS_VERSION) {
                clone_test_deps()
                dir("tests") {
                  createVenv(reqFile: "requirements.txt")
                  withVenv {
                    xcoreBuild()
                    sh "pytest -v -n auto --junitxml=pytest_result.xml"
                    junit "pytest_result.xml"
                  } // withVenv
                } // dir("tests")
              } //withTools
            } // dir("${REPO_NAME}")
          } // steps
        } // Simulator tests

        stage("Archive lib") {
            steps
            {
              archiveSandbox(REPO_NAME)
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
