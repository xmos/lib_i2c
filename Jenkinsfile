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
        dir("${SANDBOX}"){
          libraryChecks("${REPO}")
        }
      }
    }
    stage('Build') {
      steps {
        dir("${SANDBOX}") {
          xCompile("${REPO}/lib_i2c")
        }
      }
    }
    stage('Test') {
      steps {
        dir("${SANDBOX}") {
          xmostest("${REPO}", "tests")
        }
      }
    }
    stage('AppNotes') {
      steps {
        dir("${SANDBOX}") {
          allAppNotes("${REPO}/examples")
        }
      }
    }
  }
  post {
    always {
      cleanWs()
    }
  }
}
