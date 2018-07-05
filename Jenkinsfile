pipeline {
  agent {
    label 'x86&&macOS&&Apps'
  }
  environment {
    VIEW = 'swapps'
    SANDBOX = 'i2c'
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
        allAppNotes("${REPO}/examples")
      }
    }
    stage('Test') {
      steps {
        sh "echo 'Skipping the long stuff'"
        // xmostest("${REPO}", "tests")
      }
    }
  }
  post {
    always {
      cleanWs()
    }
  }
}
