# Step 1: Create the vertically stacked image
magick outputs/bonus.ppm outputs/shading.ppm outputs/normal.ppm outputs/red.ppm outputs/skybox.ppm -append collage_tmp.png

# Step 2: Create a warped heatmap effect (Middle panel)
magick collage_tmp.png -virtual-pixel mirror -spread 5 -swirl 10 -motion-blur 0x16+32 collage_heatmap.png

# Step 3: Create a pixelated glitch effect (Right panel)
magick collage_tmp.png -resize 50% -scale 200% -posterize 4 -statistic NonPeak 5 collage_glitch.png

# Step 4: Combine all three images horizontally
magick collage_heatmap.png collage_tmp.png collage_glitch.png +append collage.png

# Cleanup temporary files
rm collage_tmp.png collage_heatmap.png collage_glitch.png