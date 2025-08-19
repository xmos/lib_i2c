// This file relates to internal XMOS infrastructure and should be ignored by external users

@Library('xmos_jenkins_shared_library@v0.41.1') _

getApproval()

pipeline {

  agent none

  parameters {
    string(
      name: 'TOOLS_VERSION',
      defaultValue: '15.3.1',
      description: 'The XTC tools version'
    )
    string(
      name: 'XMOSDOC_VERSION',
      defaultValue: 'v7.4.0',
      description: 'The xmosdoc version'
    )
    string(
      name: 'INFR_APPS_VERSION',
      defaultValue: 'v3.1.1',
      description: 'The infr_apps version'
    )
  }

  options {
    buildDiscarder(xmosDiscardBuildSettings(onlyArtifacts = false))
    skipDefaultCheckout()
    timestamps()
  }

  stages {
    stage('🏗️ Build and test') {
      agent {
        label 'documentation && linux && x86_64'
      }

      stages {
        stage('Checkout') {
          steps {
            println "Stage running on ${env.NODE_NAME}"

            script {
              def (server, user, repo) = extractFromScmUrl()
              env.REPO_NAME = repo
            }

            dir(REPO_NAME) {
              checkoutScmShallow()
            }
          }
        }

        stage('Examples build') {
          steps {
            dir("${REPO_NAME}/examples") {
              xcoreBuild()
            }
          }
        }

        stage('Repo checks') {
          steps {
            warnError("Repo checks failed") {
              runRepoChecks("${WORKSPACE}/${REPO_NAME}")
            }
          }
        }

        stage('Doc build') {
          steps {
            dir(REPO_NAME) {
              buildDocs()
              
              // Todo: move app-notes
              // warnError("Documentation build failed") {
              //   buildDocs()
              //   dir("examples/AN00156_i2c_master_example") {
              //     buildDocs()
              //   }
              //   dir("examples/AN00157_i2c_slave_example") {
              //     buildDocs()
              //   }
              // }
            }
          }
        } // stage('Build Documentation')

        stage('Simulator tests') {
          steps {
            dir("${REPO_NAME}/tests") {
              withTools(params.TOOLS_VERSION) {
                createVenv(reqFile: "requirements.txt")
                withVenv {
                  xcoreBuild(archiveBins: false)
                  sh "pytest -v -n auto --junitxml=pytest_result.xml"
                }
              } //withTools
            }
          }
          post {
            always {
              junit "${REPO_NAME}/tests/pytest_result.xml"
            }
          }
        } // Simulator tests

        stage("Archive lib") {
          steps {
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
    
    stage('🚀 Release') {
      steps {
        triggerRelease()
      }
    }
  } // stages
} // pipeline
