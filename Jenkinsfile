pipeline {
  agent {
    label 'x86&&macOS&&Apps'
  }
  environment {
    VIEW = 'swapps'
    REPO = 'lib_i2c'
  }
  options {
    skipDefaultCheckout()
  }
  stages {
    stage('Get view') {
      steps {
        prepareAppsSandbox("${VIEW}", "${REPO}")
      }
    }
    stage('Library checks') {
      steps {
        libraryChecks("${REPO}")
      }
    }
    stage('App Notes') {
      steps {
        buildAllAppNotes("${REPO}/examples")
      }
    }
    stage('Test') {
      steps {
        runXmostest("${REPO}", "tests")
      }
    }
  }
  post {
    always {
      archiveArtifacts artifacts: "${REPO}/**/*.*", fingerprint: true
      cleanWs()
    }
  }
}
