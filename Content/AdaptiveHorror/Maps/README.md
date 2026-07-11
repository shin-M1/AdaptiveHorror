# Runtime prototype arena

`L_DevGym.umap` はUnreal Editorを利用できる環境で保存する予定です。現段階では `/Engine/Maps/Entry` を起動し、`AEvaPrototypeGameMode` が床、外壁、遮蔽物、照明、NavMesh Bounds、PlayerStart、通常ゾンビ、Checkpoint、Ammo／Health Pickupを実行時生成します。

これにより外部アセットなしでC++縦切りを検証できます。Editor利用時はPlay確認後に専用Mapへ置き換え、`bBuildRuntimeArena` を無効化してください。

