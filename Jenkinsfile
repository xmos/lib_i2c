@Library('xmos_jenkins_shared_library@develop') _

def clone_test_deps() {
  dir("${WORKSPACE}") {
    sh "ls ."
    sh "git clone git@github.com:xmos/test_support"
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
      defaultValue: 'v6.0.0',
      description: 'The xmosdoc version')
  }
  stages {
    stage('Build and test') {
      agent {
        label 'x86_64 && linux'
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
            }
            runLibraryChecks("${WORKSPACE}/${REPO}")
          }
        }  // Build examples

        stage('Build documentation') {
          steps {
            dir("${REPO}") {
              withXdoc("v2.0.20.2.post0") {
                withTools(params.TOOLS_VERSION) {
                  dir("${REPO}/doc") {
                    sh "xdoc xmospdf"
                    archiveArtifacts artifacts: "pdf/*.pdf"
                  }
                  forAllMatch("examples", "AN*/") { path ->
                    dir("${path}/doc")
                    {
                      sh "xdoc xmospdf"
                      archiveArtifacts artifacts: "pdf/*.pdf"
                    }
                  }
                }
              }
            }
          }
        }  // Build documentation

        stage('Simulator tests') {
          steps {
            dir("${REPO}") {
              withTools(params.TOOLS_VERSION) {
                clone_test_deps()
                createVenv(reqFile: "tests/requirements.txt")
                withVenv {
                  dir("tests") {
                    sh 'cmake -G "Unix Makefiles" -B build'
                    sh 'xmake -C build -j 8'
                    sh "pytest -n auto --junitxml=pytest_result.xml"
                  }
                }
              }
            }
          }
        } // Simulator tests
      }
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
