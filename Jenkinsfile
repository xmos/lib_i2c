@Library('xmos_jenkins_shared_library@develop') _

def clone_test_deps() {
  dir("${WORKSPACE}") {
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
      parallel {
        stage('xcore app build and run tests') {
          agent {
            label 'x86_64 && linux'
          }
          steps {
            dir("${REPO}") {
              checkout scm

              dir("examples") {
                withTools(params.TOOLS_VERSION) {
                  sh 'cmake -G "Unix Makefiles" -B build'
                  sh 'xmake -C build -j 8'
                }
              }

              runLibraryChecks("${WORKSPACE}/${REPO}", "v2.0.0")

              clone_test_deps()
              createVenv("requirements.txt")
              withVenv() {
                sh "pip install -r requirements.txt"
              }
              withTools(params.TOOLS_VERSION) {
                withVenv() {
                  dir("tests") {
                    sh 'cmake -G "Unix Makefiles" -B build'
                    sh 'xmake -C build -j 8'
                    sh "pytest -v -n auto --junitxml=pytest_unit.xml"
                  }
                }
              }
            }
          }
          post {
            cleanup {
              xcoreCleanSandbox()
            }
          }
        }
        stage('Build docs') {
          agent {
            label 'documentation'
          }
          steps {
            dir("${REPO}") {
              checkout scm

              withXdoc("v2.0.20.2.post0") {
                withTools(params.TOOLS_VERSION) {
                  dir("${REPO}/doc") {
                    sh "xdoc xmospdf"
                    archiveArtifacts artifacts: "pdf/*.pdf"
                  }
                }
              }
            }
          }
          post {
            cleanup {
              xcoreCleanSandbox()
            }
          }
        }
      }
    }
  }
}
