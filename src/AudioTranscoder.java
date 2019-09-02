import java.io.InputStream;
import java.io.OutputStream;

public class AudioTranscoder {
    public native int transcodeToPCM(InputStream inputStream, OutputStream outputStream);

    public native int transcodeToPCMFile(String readPath, String filePath);

    public static void main(String[] args) {
        System.load("../jni/transcode");
        new AudioTranscoder().transcodeToPCMFile(
                "/Users/Desktop/original.mp3",
                "/Users/Desktop/target.pcm");
    }
}
