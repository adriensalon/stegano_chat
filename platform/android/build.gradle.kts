plugins {
    alias(libs.plugins.android.application)
}

val gamePackage = "com.adriensalon.steganochat"

android {
    namespace = gamePackage
    compileSdk = 35

    defaultConfig {
        applicationId = gamePackage
        minSdk = 24
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
        debug {
            // debuggable by default
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
}
