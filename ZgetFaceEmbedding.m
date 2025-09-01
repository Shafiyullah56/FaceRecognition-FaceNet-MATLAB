function embedding = ZgetFaceEmbedding(img, facenet)
    % 1. Pastikan gambar dalam format RGB
    if size(img,3) == 1
        img = repmat(img, 1, 1, 3);
    end
    
    % 2. Resize ke 160x160
    img = imresize(img, [160 160]);

    % 3. Normalisasi ke [0,1]
    img = im2single(img); % skala piksel dari 0–255 menjadi 0–1

    % 4. Ubah ke dlarray dengan format SSC (spatial, spatial, channel)
    dlImg = dlarray(img, 'SSC');

    % 5. Prediksi embedding
    rawEmbedding = predict(facenet, dlImg);

    % 6. Konversi ke array biasa dan pastikan baris 1×128
    embedding = double(extractdata(rawEmbedding(:)))';
    
    % 7. Normalisasi (L2-normalization)
    embedding = embedding / norm(embedding + 1e-10); % hindari pembagian nol
end
