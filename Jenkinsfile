@Library('xmos_jenkins_shared_library@v0.18.0') _

getApproval()

pipeline {
  agent none
  stages {
    stage('Standard build and XS3 tests') {
      agent {
        label 'x86_64&&macOS'
      }
      environment {
        REPO = 'lib_i2c'
        VIEW = getViewName(REPO)
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
            forAllMatch("${REPO}/examples", "app_*/") { path ->
              runXmake(path)
            }
            forAllMatch("${REPO}/examples", "AN*/") { path ->
              runXmake(path)
            }
          }
        }
        stage('xCORE App XCOREAI builds') {
          steps {
            forAllMatch("${REPO}/examples", "app_*/") { path ->
              runXmake(path, '', 'XCOREAI=1')
            }
            forAllMatch("${REPO}/examples", "AN*/") { path ->
              runXmake(path, '', 'XCOREAI=1')
            }
          }
        }
        stage('Doc builds') {
          steps {
            runXdoc("${REPO}/${REPO}/doc")
            forAllMatch("${REPO}/examples", "AN*/") { path ->
              runXdoc("${path}/doc")
            }
            // Archive all the generated .pdf docs
            archiveArtifacts artifacts: "${REPO}/**/pdf/*.pdf", fingerprint: true, allowEmptyArchive: true
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
