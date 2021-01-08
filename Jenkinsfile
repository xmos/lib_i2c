@Library('xmos_jenkins_shared_library@feature/forallmatch_closure') _

getApproval()

pipeline {
  agent {
    label 'x86_64&&brew&&macOS'
  }
  environment {
    REPO = 'lib_i2c'
    VIEW = "${env.JOB_NAME.contains('PR-') ? REPO+'_'+env.CHANGE_TARGET : REPO+'_'+env.BRANCH_NAME}"
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
    stage('xCORE builds') {
      steps {
        xcoreAllAppsBuild("${REPO}/examples")
      }
    }
    stage('xCORE docs') {
      steps {
        xcoreAllAppNotesBuild("${REPO}/examples")
        runXdoc("${REPO}/${REPO}/doc")
      }
    }
    stage('Tests') {
      steps {
        runXmostest("${REPO}", 'tests')
      }
    }
  }
  post {
    success {
      updateViewfiles()
    }
    cleanup {
      xcoreCleanSandbox()
    }
  }
}
