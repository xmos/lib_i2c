@Library('xmos_jenkins_shared_library@viewEnv_update') _

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
    stage('AppNotes') {
      steps {
        appNote("${REPO}/examples/AN00156_i2c_master_example")
      }
    }
    stage('Test') {
      steps {
        xmostest("${REPO}", "tests")
      }
    }
  }
  post {
    always {
      cleanWs()
    }
  }
}
