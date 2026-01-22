using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Scanner : MonoBehaviour
{
    public int numberOfRays = 10;
    public int maxAttemptsPerRay = 30;
    public float hitDuration = 10f;
    public Mesh hitMesh;
    public Material wallMaterial;
    public Material floorMaterial;
    public Material monsterMaterial;
    public Material objectiveMaterial;
    public float hitSize = 0.05f;
    public GameObject scannerUI;
    public RectTransform scanFrame;
    public Camera cameraScanner;
    private List<Matrix4x4> wallHits = new();
    private List<Matrix4x4> floorHits = new();
    private List<Matrix4x4> monsterHits = new();
    private List<Matrix4x4> objectiveHits = new();
    private List<float> wallHitTimers = new();
    private List<float> floorHitTimers = new();
    private List<float> monsterHitTimers = new();
    private List<float> objectiveHitTimers = new();
    private List<Color> wallColors = new();
    private List<Color> floorColors = new();
    private List<Color> monsterColors = new();
    private List<Color> objectiveColors = new();
    private MaterialPropertyBlock propBlock;
    public bool shootRays = false;
    public bool visualizeSelection = false;
    public float maxDistance;
    LayerMask layerMask;
    [Range(0.0f, 1f)]
    public float squareViewportSize = 0.3f;
    public float minDistance = 0.03f;
    static Vector4[] instanceColors = new Vector4[1023];
    static Matrix4x4[] matrices = new Matrix4x4[100000];
    private float lastViewportSize = -1f;
    void Start()
    {
        propBlock = new MaterialPropertyBlock();
        layerMask = LayerMask.GetMask("Default"); //Na koji sloj utjecu zrake
        Cursor.lockState = CursorLockMode.Locked; //Zakljucaj kursor u sredinu ekrana

        RectTransform parent = scanFrame.parent as RectTransform;

        float width = squareViewportSize * parent.rect.width; //Sirina ekrana
        float height = squareViewportSize * parent.rect.height; //Visina ekrana

        scanFrame.sizeDelta = new Vector2(width, height);
        scanFrame.anchoredPosition = Vector2.zero; // centrirano
    }

    void Update()
    {
        if (Input.GetMouseButton(0)){ //Ako drzi lijevi klik onda pucaj zrake
            shootRays = true;
        }
        if (visualizeSelection)
        {
            scannerUI.SetActive(Input.GetMouseButton(0)); //Aktiviraj UI za skener
        }

        //Azuriraj ui skenera
        if (!Mathf.Approximately(lastViewportSize, squareViewportSize))
        {
            UpdateScanFrame();
            lastViewportSize = squareViewportSize;
        }
    }

    //Azuriraj ui skenera
    void UpdateScanFrame()
    {
        scanFrame.anchorMin = new Vector2(0.5f - squareViewportSize / 2f, 0.5f - squareViewportSize / 2f);
        scanFrame.anchorMax = new Vector2(0.5f + squareViewportSize / 2f, 0.5f + squareViewportSize / 2f);
        scanFrame.offsetMin = Vector2.zero;
        scanFrame.offsetMax = Vector2.zero;
    }

    void FixedUpdate()
    {
        //Ako igrac puca zrake
        if (shootRays){
            shootRays = false;
            Camera camera = cameraScanner; //Referenca na kameru igraca
            int maxAttempts = maxAttemptsPerRay; //Broj pokusaja za ispucavanje zrake po jednom pritisku
            List<Vector2> viewportPoints = new(); //Lista valjanih hit tocaka

            //Sve dok je broj tocaka u listi manji od ogranicenja i dok se nije
            while (viewportPoints.Count < numberOfRays * numberOfRays && maxAttempts > 0){

                //Uzimanje random tocke unutar kvadrata skenera
                float viewX = Random.Range(0.5f - squareViewportSize / 2f, 0.5f + squareViewportSize / 2f);
                float viewY = Random.Range(0.5f - squareViewportSize / 2f, 0.5f + squareViewportSize / 2f);

                //Vector2 kandidata
                Vector2 candidate = new(viewX, viewY);

                //Ako je kandidat pre blizu vec neke druge tocke onda ga odbaci
                bool tooClose = false;
                foreach (var point in viewportPoints){
                    if (Vector2.Distance(candidate, point) < minDistance){
                        tooClose = true;
                        break;
                    }
                }

                //Inace dodaj kandidata u listu
                if (!tooClose){
                    viewportPoints.Add(candidate);
                }

                maxAttempts--;
            }

            //Za svakog kandidata ispucaj zraku
            foreach (var point in viewportPoints){

                Ray ray = camera.ViewportPointToRay(new Vector3(point.x, point.y, 0));

                //Bacanje zrake
                if (Physics.Raycast(ray, out RaycastHit hit, Mathf.Infinity, layerMask)){
                    //Sudar sa zidom
                    if (hit.transform.CompareTag("Wall")){
                        //Dodaj tocku sudara u listu
                        Matrix4x4 matrix = Matrix4x4.TRS(hit.point, Quaternion.identity, Vector3.one * hitSize);
                        wallHits.Add(matrix);
                        wallHitTimers.Add(hitDuration);
                        float udaljenostOdIgraca = Mathf.InverseLerp(0f, maxDistance, hit.distance);
                        wallColors.Add(Color.Lerp(Color.red, Color.blue, udaljenostOdIgraca));
                    }

                    //SUdar sa podom
                    else if (hit.transform.CompareTag("Floor")){
                        //Dodaj tocku sudara u listu
                        Matrix4x4 matrix = Matrix4x4.TRS(hit.point, Quaternion.identity, Vector3.one * hitSize);
                        floorHits.Add(matrix);
                        floorHitTimers.Add(hitDuration);
                        float udaljenostOdIgracat = Mathf.InverseLerp(0f, maxDistance, hit.distance);
                        //Color purple = new Color(0.5f, 0f, 1f);
                        floorColors.Add(Color.Lerp(Color.yellow, Color.gray, udaljenostOdIgracat));
                    }
                    else if (hit.transform.CompareTag("Monster")){
                        //Dodaj tocku sudara u listu
                        Matrix4x4 matrix = Matrix4x4.TRS(hit.point, Quaternion.identity, Vector3.one * hitSize);
                        monsterHits.Add(matrix);
                        monsterHitTimers.Add(hitDuration);
                        monsterColors.Add(new Color(1f, 0f, 0f, 1f));
                    }
                    else if (hit.transform.CompareTag("Objective")){
                        //Dodaj tocku sudara u listu
                        Matrix4x4 matrix = Matrix4x4.TRS(hit.point, Quaternion.identity, Vector3.one * hitSize);
                        objectiveHits.Add(matrix);
                        objectiveHitTimers.Add(hitDuration);
                        objectiveColors.Add(new Color(0f, 1f, 0f, 1f));
                    }
                }
            }
        }
    }
        

    void LateUpdate()
    {
        //Projdi kroz listu i smanji vrijeme prikaza za zid
        for (int i = wallHitTimers.Count - 1; i >= 0; i--) {
            wallHitTimers[i] -= Time.deltaTime;
            float alfa = wallHitTimers[i] / hitDuration; //Izracun transparentnosti
            wallColors[i] = new Color(wallColors[i].r, wallColors[i].g, wallColors[i].b, alfa);

            //Uklanjanje onih tocaka cije je vrijeme isteklo
            if (wallHitTimers[i] <= 0f){
                wallHitTimers.RemoveAt(i);
                wallHits.RemoveAt(i);
                wallColors.RemoveAt(i);
            }
        }

        //Projdi kroz listu i smanji vrijeme prikaza za pod
        for (int i = floorHitTimers.Count - 1; i >= 0; i--) {
            floorHitTimers[i] -= Time.deltaTime;
            float alfa = floorHitTimers[i] / hitDuration; //Izracun transparentnosti
            floorColors[i] = new Color(floorColors[i].r, floorColors[i].g, floorColors[i].b, alfa);

            //Uklanjanje onih tocaka cije je vrijeme isteklo
            if (floorHitTimers[i] <= 0f){
                floorHitTimers.RemoveAt(i);
                floorHits.RemoveAt(i);
                floorColors.RemoveAt(i);
            }
        }

        //Projdi kroz listu i smanji vrijeme prikaza za cudoviste
        for (int i = monsterHitTimers.Count - 1; i >= 0; i--) {
            monsterHitTimers[i] -= Time.deltaTime;
            float alfa = monsterHitTimers[i] / hitDuration; //Izracun transparentnosti
            monsterColors[i] = new Color(monsterColors[i].r, monsterColors[i].g, monsterColors[i].b, alfa);

            //Uklanjanje onih tocaka cije je vrijeme isteklo
            if (monsterHitTimers[i] <= 0f){
                monsterHitTimers.RemoveAt(i);
                monsterHits.RemoveAt(i);
                monsterColors.RemoveAt(i);
            }
        }

        //Projdi kroz listu i smanji vrijeme prikaza za objective
        for (int i = objectiveHitTimers.Count - 1; i >= 0; i--) {
            objectiveHitTimers[i] -= Time.deltaTime;
            float alfa = objectiveHitTimers[i] / hitDuration; //Izracun transparentnosti
            objectiveColors[i] = new Color(objectiveColors[i].r, objectiveColors[i].g, objectiveColors[i].b, alfa);

            //Uklanjanje onih tocaka cije je vrijeme isteklo
            if (objectiveHitTimers[i] <= 0f){
                objectiveHitTimers.RemoveAt(i);
                objectiveHits.RemoveAt(i);
                objectiveColors.RemoveAt(i);
            }
        }

        //Iscrtaj tocke
        const int BATCH_SIZE = 1023;

        int totalWall = wallHits.Count;
        int totalFloor = floorHits.Count;
        int totalMonster = monsterHits.Count;
        int totalObjective = objectiveHits.Count;
        int offsetWall = 0;
        int offsetFloor = 0;
        int offsetMonster = 0;
        int offsetObjective = 0;

        //Iscrtavaj sve dok ima tocaka za iscrtati
        while (offsetWall < totalWall)
        {
            //Moguce je iscrtati maksimalno 1023 objekata u jednom pozivu
            int count = Mathf.Min(BATCH_SIZE, totalWall - offsetWall);

            //Za svaku tocku kopiraj njenu TRS matricu i boju
            for (int i = 0; i < count; i++)
            {
                matrices[i] = wallHits[offsetWall + i];
                instanceColors[i] = wallColors[offsetWall + i];
            }

            //Posalji u propBlock boju za materijal
            propBlock.Clear();
            propBlock.SetVectorArray("_InstanceColor", instanceColors);

            //Poziv iscrtavvanjaa na GPU
            Graphics.DrawMeshInstanced(
                hitMesh,
                0,
                wallMaterial,
                matrices,
                count,
                propBlock
            );

            offsetWall += count;
        }

        //Iscrtavaj sve dok ima tocaka za iscrtati
        while (offsetFloor < totalFloor)
        {
            //Moguce je iscrtati maksimalno 1023 objekata u jednom pozivu
            int count = Mathf.Min(BATCH_SIZE, totalFloor - offsetFloor);

            //Za svaku tocku kopiraj njenu TRS matricu i boju
            for (int i = 0; i < count; i++)
            {
                matrices[i] = floorHits[offsetFloor + i];
                instanceColors[i] = floorColors[offsetFloor + i];
            }

            //Posalji u propBlock boju za materijal
            propBlock.Clear();
            propBlock.SetVectorArray("_InstanceColor", instanceColors);

            //Poziv iscrtavvanjaa na GPU
            Graphics.DrawMeshInstanced(
                hitMesh,
                0,
                floorMaterial,
                matrices,
                count,
                propBlock
            );

            offsetFloor += count;
        }

        //Iscrtavaj sve dok ima tocaka za iscrtati
        while (offsetMonster < totalMonster)
        {
            //Moguce je iscrtati maksimalno 1023 objekata u jednom pozivu
            int count = Mathf.Min(BATCH_SIZE, totalMonster - offsetMonster);

            //Za svaku tocku kopiraj njenu TRS matricu i boju
            for (int i = 0; i < count; i++)
            {
                matrices[i] = monsterHits[offsetMonster + i];
                instanceColors[i] = monsterColors[offsetMonster + i];
            }

            //Posalji u propBlock boju za materijal
            propBlock.Clear();
            propBlock.SetVectorArray("_InstanceColor", instanceColors);

            //Poziv iscrtavvanjaa na GPU
            Graphics.DrawMeshInstanced(
                hitMesh,
                0,
                monsterMaterial,
                matrices,
                count,
                propBlock
            );

            offsetMonster += count;
        }

        //Iscrtavaj sve dok ima tocaka za iscrtati
        while (offsetObjective < totalObjective)
        {
            //Moguce je iscrtati maksimalno 1023 objekata u jednom pozivu
            int count = Mathf.Min(BATCH_SIZE, totalObjective - offsetObjective);

            //Za svaku tocku kopiraj njenu TRS matricu i boju
            for (int i = 0; i < count; i++)
            {
                matrices[i] = objectiveHits[offsetObjective + i];
                instanceColors[i] = objectiveColors[offsetObjective + i];
            }

            //Posalji u propBlock boju za materijal
            propBlock.Clear();
            propBlock.SetVectorArray("_InstanceColor", instanceColors);

            //Poziv iscrtavvanjaa na GPU
            Graphics.DrawMeshInstanced(
                hitMesh,
                0,
                objectiveMaterial,
                matrices,
                count,
                propBlock
            );

            offsetObjective += count;
        }
    }
}
