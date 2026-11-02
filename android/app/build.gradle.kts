plugins {
    id("com.android.application")
}

android {
    namespace = "app.lifecounter"
    compileSdk = 36

    defaultConfig {
        applicationId = "app.lifecounter"
        minSdk = 30  // Android 11。無線デバッグ対応と同じ。WindowInsetsController を直接使うため
        targetSdk = 36
        versionCode = 1
        versionName = "0.1"
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_21
        targetCompatibility = JavaVersion.VERSION_21
    }
}

dependencies {
    implementation(project(":core"))
}
