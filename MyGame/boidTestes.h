#pragma once


/*

for (size_t i = 0; i < boidCount; i++)
	{
		boids[i].pos = vec2(i / 10, i % 10) * 40.0f + 50.0f;
		float angle = ran(0, 1000);
		boids[i].velocity = vec2(cosf(angle), sinf(angle)) * 500.0f;
	}

	bool recreateBoidCells = true;
	int cellsX;
	int cellsY;
	int cellCount;
	float cellSize;




	// main loop


	// define cell layout
			{
				if (recreateBoidCells || engine.WindowResizedLastFrame()) {
					float minCellSize = boidSettings.visualRange;
					cellSize = minCellSize + 5;

					cellsX = (engine.winW / cellSize + 1);
					cellsY = (engine.winH / cellSize + 1);
					cellCount = cellsX * cellsY;

					boidCells.resize(cellCount);
				}
				recreateBoidCells = false;
			}

			{
				for (auto& b : boids)
				{
					int currentCellX = (int)(b.pos.x / cellSize);
					int currentCellY = (int)(b.pos.y / cellSize);

					// only matters if boid goes out of bounds
					currentCellX = glm::clamp(currentCellX, 0, cellsX - 1);
					currentCellY = glm::clamp(currentCellY, 0, cellsY - 1);

					b.cellIndex = currentCellX + currentCellY * cellsX;
				}

				std::sort(boids.begin(), boids.end(), [](const Boid& a, const Boid& b) {
					return a.cellIndex < b.cellIndex;
					});

				for (auto& cell : boidCells)
				{
					cell.startIndex = 0;
					cell.endIndex = 0;
				}

				int cellIndex = boids[0].cellIndex;
				int lastBoidIndex = 0;
				boidCells[cellIndex].startIndex = 0;
				for (size_t i = 1; i < boids.size(); i++)
				{
					if (boids[i].cellIndex != cellIndex)
					{
						boidCells[cellIndex].endIndex = i;
						boidCells[boids[i].cellIndex].startIndex = i;
						cellIndex = boids[i].cellIndex;
					}
				}
			}


			//// bin boids into cells
			//{
			//	int indexInCell = 0;
			//	for (size_t i = 0; i < boidCells.size(); i++)
			//	{
			//		float cellX1 = (i % cellsX) * cellSize;
			//		float cellX2 = cellX1 + cellSize;
			//		float cellY1 = (i / cellsX) * cellSize;
			//		float cellY2 = cellY1 + cellSize;

			//		BoidCell& cell = boidCells[i];

			//		cell.startIndex = indexInCell;

			//		for (size_t j = 0; j < boidCount; j++)
			//		{
			//			if (boids[j].pos.x >= cellX1 && boids[j].pos.x < cellX2 && boids[j].pos.y >= cellY1 && boids[j].pos.y < cellY2) {
			//				boidBinIndexes[indexInCell] = j;
			//				indexInCell++;
			//			}
			//		}

			//		cell.endIndex = indexInCell;
			//	}
			//	//assert(indexInCell == boidCount);
			//}

			boidSettings.bounds = vec2(engine.winW, engine.winH);

			std::for_each(std::execution::par_unseq, boids.begin(), boids.end(), [&engine, &cellSize, &cellsX, &cellsY](Boid& b) {

				//for (auto& b : boids)
				{

					// inter bird behaviour
					{
						vec2 center = vec2(0);
						vec2 avoidVector = vec2(0);
						vec2 avgVelocity = vec2(0.0f);
						int numNeighbors = 0;

						int currentCellX = (int)(b.pos.x / cellSize);
						int currentCellY = (int)(b.pos.y / cellSize);

						// only matters if boid goes out of bounds
						currentCellX = glm::clamp(currentCellX, 0, cellsX - 1);
						currentCellY = glm::clamp(currentCellY, 0, cellsY - 1);

						for (int cx = -1; cx < 2; cx++)
						{
							for (int cy = -1; cy < 2; cy++)
							{
								int cellx = (currentCellX + cx);
								int celly = (currentCellY + cy);
								if (cellx < 0 || cellx > cellsX - 1 || celly < 0 || celly > cellsY - 1)
									continue;
								int cellIndex = cellx + celly * cellsX;

								BoidCell cell = boidCells[cellIndex];

								for (size_t j = cell.startIndex; j < cell.endIndex; j++)
								{
									//Boid& ob = boids[boidBinIndexes[j]];
									Boid& ob = boids[j];

									float dist = glm::length(b.pos - ob.pos);
									if (dist < boidSettings.visualRange) {
										center += ob.pos;
										avgVelocity += ob.velocity;
										numNeighbors++;
									}

									if (&b != &ob)
									{
										if (dist < boidSettings.minDistance)
											avoidVector += b.pos - ob.pos;
									}
								}
							}
						}

						//for (auto& ob : boids)
						//{
						//	float dist = glm::length(b.pos - ob.pos);
						//	if (dist < boidSettings.visualRange) {
						//		center += ob.pos;
						//		avgVelocity += ob.velocity;
						//		numNeighbors++;
						//	}

						//	if (&b != &ob)
						//	{
						//		if (length(b.pos - ob.pos) < boidSettings.minDistance)
						//			avoidVector += b.pos - ob.pos;
						//	}
						//}

						if (numNeighbors > 0) {
							center = center / (float)numNeighbors;
							b.velocity += (center - b.pos) * boidSettings.centeringFactor * (float)engine.deltaTime;

							avgVelocity = avgVelocity / (float)numNeighbors;
							b.velocity += (avgVelocity - b.velocity) * boidSettings.matchingFactor * (float)engine.deltaTime;
						}

						b.velocity += avoidVector * boidSettings.avoidFactor * (float)engine.deltaTime;
					}


					// gentle acceleration
					{
						vec2 n = glm::normalize(b.velocity);
						b.velocity += n * boidSettings.generalAccel * (float)engine.deltaTime;
					}

					// limit speed
					{
						float speed = magnitude(b.velocity);
						if (speed > boidSettings.maxSpeed) {
							float d = boidSettings.maxSpeed / speed;
							b.velocity = (b.velocity / speed) * boidSettings.maxSpeed;
						}
					}

					// keep within bounds
					{
						float delta = boidSettings.turnFactor * engine.deltaTime;
						if (b.pos.x < boidSettings.margin) {
							b.velocity.x += delta;
						}
						if (b.pos.x > boidSettings.bounds.x - boidSettings.margin) {
							b.velocity.x -= delta;
						}
						if (b.pos.y < boidSettings.margin) {
							b.velocity.y += delta;
						}
						if (b.pos.y > boidSettings.bounds.y - boidSettings.margin) {
							b.velocity.y -= delta;
						}
					}
				}
				});


			for (auto& b : boids) {
				b.pos += b.velocity * (float)engine.deltaTime * 1.0f;
				engine.addScreenSpaceQuad(vec4(1, 0, 0, 1), b.pos, vec2(10, 5), atan2f(b.velocity.y, b.velocity.x));
			}




// boid settings window

//ImGui::Begin("boids settings");
				//ImGui::SliderFloat("centering", &boidSettings.centeringFactor, 0.001f, 1.0f);
				//recreateBoidCells |= ImGui::SliderFloat("visual range", &boidSettings.visualRange, 0.0f, 1000.0f);
				//ImGui::SliderFloat("min distance", &boidSettings.minDistance, 0.0f, boidSettings.visualRange);
				//ImGui::SliderFloat("avoid factor", &boidSettings.avoidFactor, 0.0f, 10.0f);
				//ImGui::SliderFloat("match factor", &boidSettings.matchingFactor, 0.0f, 10.0f);
				//ImGui::SliderFloat("general acceleration", &boidSettings.generalAccel, 0.0f, 50.0f);
				//ImGui::SliderFloat("max speed", &boidSettings.maxSpeed, 0.0f, 1000.0f);
				//ImGui::SliderFloat("margin", &boidSettings.margin, 0.0f, 500.0f);
				//ImGui::SliderFloat("turn factor", &boidSettings.turnFactor, 0.0f, 1400.0f);
				//ImGui::End();


*/