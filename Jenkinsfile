// This file relates to internal XMOS infrastructure and should be ignored by external users

@Library('xmos_jenkins_shared_library@v0.34.0') _

def clone_test_deps() {
  dir("${WORKSPACE}") {
    sh "git clone git@github.com:xmos/test_support"
    sh "git -C test_support checkout v2.0.0"
  }
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
      defaultValue: 'v6.1.0',
      description: 'The xmosdoc version')
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
              checkout scm

              dir("examples") {
                withTools(params.TOOLS_VERSION) {
                  sh 'cmake -G "Unix Makefiles" -B build'
                  sh 'xmake -C build -j 8'
                }
              }
            } // dir("${REPO}")
          } // steps
        }  // stage('Build examples')

        stage('Library Checks') {
          steps {
            runLibraryChecks("${WORKSPACE}/${REPO}", "v2.0.1")
          } // steps
        } // stage('Library Checks')

        stage('Build Documentation') {
          steps {
            dir("${REPO}") {
              warnError("Docs") {
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
                    sh 'cmake -G "Unix Makefiles" -B build'
                    sh 'xmake -C build -j 8'
                    sh "pytest -v -n auto --junitxml=pytest_result.xml"
                  } // withVenv
                } // dir("tests")
              } //withTools
            } // dir("${REPO}")
          } // steps
        } // Simulator tests
      } // stages
      post {
        always {
          junit "${REPO}/tests/pytest_result.xml"
        }
        cleanup {
          xcoreCleanSandbox()
        }
      }
    }  // Build and test
  }
}
