@Library('xmos_jenkins_shared_library@feature/support_xcoreai_2') _

getApproval()

pipeline {
  agent none
  stages {
    stage('Standard build and XS3 tests') {
      agent {
        label 'x86_64&&brew&&macOS'
      }
      environment {
        REPO = 'lib_i2c'
        VIEW = "lib_i2c_develop_tools_15"
      }
      options {
        skipDefaultCheckout()
      }
      stages {
        stage('Get view') {
          steps {
            xcorePrepareSandbox("${VIEW}", "${REPO}")
          }
        }
        stage('Library checks') {
          steps {
             xcoreLibraryChecks("${REPO}")
          }
        }
        stage('xCORE App XS2 builds') {
          steps {
            dir("${REPO}") {
              xcoreAllAppsBuild('examples')
              xcoreAllAppNotesBuild('examples')
            }
          }
        }
        stage('xCORE App XCOREAI builds') {
          steps {
            dir("${REPO}") {
              xcoreAllAppsBuild('examples', '', 'XCOREAI=1')
              xcoreAllAppNotesBuild('examples', '', 'XCOREAI=1')
            }
          }
        }
        stage('Lib Doc build') {
          steps {
            dir("${REPO}/${REPO}") {
              runXdoc(doc')
            }
          }
        }
        stage('Tests XS1, XS2 and XCOREAI') {
          steps {
            runXmostest("${REPO}", 'tests')
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
  post {
    success {
      node("linux") {
        updateViewfiles()
        xcoreCleanSandbox()
      }
    }
  }
}
