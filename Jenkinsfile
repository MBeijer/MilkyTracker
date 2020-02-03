def notify(status){
	emailext (
		body: '$DEFAULT_CONTENT',
		recipientProviders: [
			[$class: 'CulpritsRecipientProvider'],
			[$class: 'DevelopersRecipientProvider'],
			[$class: 'RequesterRecipientProvider']
		],
		replyTo: '$DEFAULT_REPLYTO',
		subject: '$DEFAULT_SUBJECT',
		to: '$DEFAULT_RECIPIENTS'
	)
}

@NonCPS
def killall_jobs() {
	def jobname = env.JOB_NAME
	def buildnum = env.BUILD_NUMBER.toInteger()
	def killnums = ""
	def job = Jenkins.instance.getItemByFullName(jobname)
	def fixed_job_name = env.JOB_NAME.replace('%2F','/')

	for (build in job.builds) {
		if (!build.isBuilding()) { continue; }
		if (buildnum == build.getNumber().toInteger()) { continue; println "equals" }
		if (buildnum < build.getNumber().toInteger()) { continue; println "newer" }

		echo "Kill task = ${build}"

		killnums += "#" + build.getNumber().toInteger() + ", "

		build.doStop();
	}

	if (killnums != "") {
		slackSend color: "danger", channel: "#jenkins", message: "Killing task(s) ${fixed_job_name} ${killnums} in favor of #${buildnum}, ignore following failed builds for ${killnums}"
	}
	echo "Done killing"
}

def buildStep(dockerImage, os) {
	def fixed_job_name = env.JOB_NAME.replace('%2F','/')
	def commondir = env.WORKSPACE + '/../' + env.JOB_NAME.replace('%2F','/') + '/'
	
	try{
		stage("Building on \"${dockerImage}\" for \"${os}\"...") {
			properties([pipelineTriggers([githubPush()])])

			def dockerImageRef = docker.image("${dockerImage}")
			dockerImageRef.pull()
			dockerImageRef.inside("-u 0:0 -u root -w /tmp/work") {

				sh "apt update"
				sh "apt install -y curl automake autoconf libtool unzip"
				
				checkout scm
		
				if (env.CHANGE_ID) {
					echo 'Trying to build pull request'
				}

				if (!env.CHANGE_ID) {
					sh "rm -rfv ${env.WORKSPACE}/publishing/deploy/*"
					sh "mkdir -p ${env.WORKSPACE}/publishing/deploy/milkytracker"
				}
				
				sh "mkdir -p build/"
				sh "mkdir -p lib/"
				sh "rm -rfv build/*"

				slackSend color: "good", channel: "#jenkins", message: "Starting ${os} build target..."
				
				try {
					sh "platforms/${os}/prep.sh"
				} catch(err) {
					notify('Prep not found')
				}
				
				dir("build") {
					sh "PKG_CONFIG_PATH=/opt/${os}/lib/pkgconfig/:/opt/${os}/share/pkgconfig/ cmake -DM68K_CPU=68040 -DM68K_FPU=hard .."
					def _NPROCESSORS_ONLN = sh (
						script: 'getconf _NPROCESSORS_ONLN',
						returnStdout: true
					).trim()
					sh "VERBOSE=1 cmake --build . --config Release -- -j${_NPROCESSORS_ONLN}"
					
					sh "cp src/tracker/milkytracker milkytracker-${os}"
					archiveArtifacts artifacts: "milkytracker-${os}"
				}
				
				if (!env.CHANGE_ID) {
					sh "mkdir -p ${env.WORKSPACE}/publishing/deploy/milkytracker/${os}/"

					sh "echo '${env.BUILD_NUMBER}|${env.BUILD_URL}' > ${env.WORKSPACE}/publishing/deploy/milkytracker/${os}/BUILD"

					sh "cp -fvr build/src/tracker/milkytracker ${env.WORKSPACE}/publishing/deploy/milkytracker/${os}/"
					//sh "cd ${env.WORKSPACE}/publishing/deploy/milkytracker/${os}/ && tar -Jcvvf ${ext}${sfx}-hosted.tar.xz *"
				}
				stash includes: "publishing/deploy/milkytracker/**", name: "${os}"
				slackSend color: "good", channel: "#jenkins", message: "Build ${fixed_job_name} #${env.BUILD_NUMBER} Target: ${os} DockerImage: ${dockerImage} successful!"
			}
		}
	} catch(err) {
		slackSend color: "danger", channel: "#jenkins", message: "Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER} Target: ${os} DockerImage: ${dockerImage} (<${env.BUILD_URL}|Open>)"
		currentBuild.result = 'FAILURE'
		notify('Build failed')
		throw err
	}
}

node('master') {
	killall_jobs()
	def fixed_job_name = env.JOB_NAME.replace('%2F','/')
	slackSend color: "good", channel: "#jenkins", message: "Build Started: ${fixed_job_name} #${env.BUILD_NUMBER} (<${env.BUILD_URL}|Open>)"
	parallel (
		'Build m68k-amigaos': {
			node {
				buildStep('amigadev/crosstools:m68k-amigaos', 'm68k-amigaos')
			}
		}
		/*,
		'Build Linux Hosted x86_64 version - GCC 9.1.0 - Binutils 2.32': {
			node {
				buildStep('linux-x86_64', 'GorillaSmall', '2.32', '9.1.0', false, 'contrib-installerlg')
			}
		}*/
	)

	stage("Publishing") {
		sh "rm -rfv publishing/"

		try {
			// ${os}
			unstash "m68k-amigaos"
		} catch(err) {
			notify('Stash not found')
		}

		if (env.TAG_NAME) {
			sh "echo $TAG_NAME > publishing/deploy/STABLE"
			sh "ssh $DEPLOYHOST mkdir -p public_html/downloads/releases/milkytracker/$TAG_NAME"
			sh "scp -r publishing/deploy/milkytracker/* $DEPLOYHOST:~/public_html/downloads/releases/milkytracker/$TAG_NAME/"
			sh "scp publishing/deploy/STABLE $DEPLOYHOST:~/public_html/downloads/releases/milkytracker/"
		} else if (env.BRANCH_NAME.equals('master')) {
			def deploy_url = sh (
				script: 'echo "nightly/milkytracker/`date +\'%Y\'`/`date +\'%m\'`/`date +\'%d\'`/"',
				returnStdout: true
			).trim()
			sh "date +'%Y-%m-%d %H:%M:%S' > publishing/deploy/BUILDTIME"
			sh "ssh $DEPLOYHOST mkdir -p public_html/downloads/nightly/milkytracker/`date +'%Y'`/`date +'%m'`/`date +'%d'`/"
			sh "scp -r publishing/deploy/milkytracker/* $DEPLOYHOST:~/public_html/downloads/nightly/milkytracker/`date +'%Y'`/`date +'%m'`/`date +'%d'`/"
			sh "scp publishing/deploy/BUILDTIME $DEPLOYHOST:~/public_html/downloads/nightly/milkytracker/"

			slackSend color: "good", channel: "#jenkins", message: "New ${fixed_job_name} build #${env.BUILD_NUMBER} to web (<https://dl.amigadev.com/${deploy_url}|https://dl.amigadev.com/${deploy_url}>)"
			//ircNotify targets: "#aros_caffeine", customMessage: "New ${fixed_job_name} build #${env.BUILD_NUMBER} to web: https://dl.amigadev.com/${deploy_url}"
		}
	}
}
